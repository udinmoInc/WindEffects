#include "Environment/EnvironmentEditorApi.h"

#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentTypes.h"
#include "Environment/EnvironmentTypes.h"
#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Widgets/PropertyEditor.hpp"
#include "Widgets/TreeView.hpp"
#include "Widgets/MenuBar.hpp"
#include "Layout/OverlayManager.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>
#include <cstdio>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::environment {

namespace {

using we::runtime::scene::Entity;
using we::runtime::scene::EntityType;
using we::runtime::scene::Scene;
using we::runtime::world::environment::EnvironmentActorKind;
using we::runtime::world::environment::EnvironmentPreset;
using we::runtime::world::environment::EnvironmentSystem;

std::weak_ptr<Scene> g_Scene;
std::weak_ptr<we::UI::TreeView> g_Outliner;
std::weak_ptr<we::UI::PropertyEditor> g_Details;
std::uint64_t g_LastSelectedEntityId = 0;

std::string EntityTypeLabel(EntityType type) {
    switch (type) {
    case EntityType::DirectionalLight: return "Directional Light";
    case EntityType::SkyLight: return "Sky Light";
    case EntityType::SkyAtmosphere: return "Sky Atmosphere";
    case EntityType::HeightFog: return "Exponential Height Fog";
    case EntityType::VolumetricClouds: return "Volumetric Clouds";
    case EntityType::EmptyActor: return "Folder";
    default: return "Actor";
    }
}

std::string IconForEntity(const Entity& entity) {
    switch (entity.Type) {
    case EntityType::DirectionalLight:
        return we::UI::Icons::SunName;
    case EntityType::SkyLight:
        return we::UI::Icons::LightName;
    case EntityType::SkyAtmosphere:
        return we::UI::Icons::SphereName;
    case EntityType::HeightFog:
        return we::UI::Icons::LayersName;
    case EntityType::VolumetricClouds:
        return we::UI::Icons::SphereName;
    case EntityType::EmptyActor:
        return we::UI::Icons::FolderName;
    default:
        return we::UI::Icons::CubeName;
    }
}

int EnvironmentActorSortKey(const Entity& entity) {
    using we::runtime::world::environment::kHeightFogActorName;
    using we::runtime::world::environment::kSkyAtmosphereActorName;
    using we::runtime::world::environment::kSkyLightActorName;
    using we::runtime::world::environment::kSunActorName;
    using we::runtime::world::environment::kVolumetricCloudsActorName;
    using we::runtime::world::environment::kExposureControllerActorName;

    if (entity.Name == kSunActorName || entity.Name == "Sun Light" || entity.Type == EntityType::DirectionalLight) {
        return 0;
    }
    if (entity.Name == kSkyLightActorName || entity.Name == "Sky Light" || entity.Type == EntityType::SkyLight) {
        return 1;
    }
    if (entity.Name == kSkyAtmosphereActorName || entity.Name == "Sky Atmosphere" || entity.Type == EntityType::SkyAtmosphere) {
        return 2;
    }
    if (entity.Name == kHeightFogActorName || entity.Name == "Exponential Height Fog" || entity.Type == EntityType::HeightFog) {
        return 3;
    }
    if (entity.Name == kVolumetricCloudsActorName || entity.Type == EntityType::VolumetricClouds) {
        return 4;
    }
    if (entity.Name == kExposureControllerActorName) {
        return 5;
    }
    return 100;
}

void SortTreeChildren(std::vector<std::shared_ptr<we::UI::TreeNode>>& children, Scene& scene) {
    std::sort(children.begin(), children.end(), [&](const std::shared_ptr<we::UI::TreeNode>& a, const std::shared_ptr<we::UI::TreeNode>& b) {
        const Entity* entityA = scene.FindEntityById(static_cast<std::uint64_t>(std::strtoull(a->id.c_str(), nullptr, 10)));
        const Entity* entityB = scene.FindEntityById(static_cast<std::uint64_t>(std::strtoull(b->id.c_str(), nullptr, 10)));
        if (!entityA || !entityB) {
            return a->label < b->label;
        }

        const int keyA = EnvironmentActorSortKey(*entityA);
        const int keyB = EnvironmentActorSortKey(*entityB);
        if (keyA != keyB) {
            return keyA < keyB;
        }
        return entityA->Name < entityB->Name;
    });

    for (const auto& child : children) {
        if (!child->children.empty()) {
            SortTreeChildren(child->children, scene);
        }
    }
}

std::string FormatFloat(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    return buffer;
}

std::string FormatVec3(const glm::vec3& value) {
    return FormatFloat(value.x) + "," + FormatFloat(value.y) + "," + FormatFloat(value.z);
}

glm::vec3 ParseVec3(const std::string& text, const glm::vec3& fallback) {
    glm::vec3 result = fallback;
    char comma = 0;
    if (std::sscanf(text.c_str(), "%f%c%f%c%f", &result.x, &comma, &result.y, &comma, &result.z) >= 1) {
        return result;
    }
    return fallback;
}

float ParseFloat(const std::string& text, float fallback) {
    try {
        return std::stof(text);
    } catch (...) {
        return fallback;
    }
}

int ParseInt(const std::string& text, int fallback) {
    try {
        return std::stoi(text);
    } catch (...) {
        return fallback;
    }
}

void AddBoolProperty(
    we::UI::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    bool value,
    std::function<void(bool)> onChanged) {
    we::UI::Property property;
    property.name = name;
    property.category = category;
    property.type = we::UI::PropertyType::Bool;
    property.value = value ? "true" : "false";
    property.defaultValue = property.value;
    property.onValueChanged = [onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(newValue == "true" || newValue == "1");
        }
    };
    editor.AddProperty(property);
}

void AddFloatProperty(
    we::UI::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    float value,
    std::function<void(float)> onChanged) {
    we::UI::Property property;
    property.name = name;
    property.category = category;
    property.type = we::UI::PropertyType::Float;
    property.value = FormatFloat(value);
    property.defaultValue = property.value;
    property.onValueChanged = [onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(ParseFloat(newValue, 0.0f));
        }
    };
    editor.AddProperty(property);
}

void AddIntProperty(
    we::UI::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    int value,
    std::function<void(int)> onChanged) {
    we::UI::Property property;
    property.name = name;
    property.category = category;
    property.type = we::UI::PropertyType::Int;
    property.value = std::to_string(value);
    property.defaultValue = property.value;
    property.onValueChanged = [onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(ParseInt(newValue, 0));
        }
    };
    editor.AddProperty(property);
}

void AddVec3Property(
    we::UI::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    const glm::vec3& value,
    std::function<void(const glm::vec3&)> onChanged) {
    we::UI::Property property;
    property.name = name;
    property.category = category;
    property.type = we::UI::PropertyType::Vector3;
    property.value = FormatVec3(value);
    property.defaultValue = property.value;
    property.onValueChanged = [value, onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(ParseVec3(newValue, value));
        }
    };
    editor.AddProperty(property);
}

void BindSunProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& sun = system.GetSun();
    AddFloatProperty(editor, "Intensity", "Light", sun.Intensity, [&system](float value) {
        system.GetSun().Intensity = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddIntProperty(editor, "Temperature", "Light", sun.TemperatureKelvin, [&system](int value) {
        system.GetSun().TemperatureKelvin = std::max(1000, value);
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddBoolProperty(editor, "Cast Dynamic Shadows", "Light", sun.CastDynamicShadows, [&system](bool value) {
        system.GetSun().CastDynamicShadows = value;
        system.UpdateRendering();
    });
    AddBoolProperty(editor, "Atmosphere Sun", "Light", sun.AtmosphereSun, [&system](bool value) {
        system.GetSun().AtmosphereSun = value;
        system.UpdateRendering();
    });
    AddVec3Property(editor, "Rotation", "Transform", sun.Rotation, [&system](const glm::vec3& value) {
        system.GetSun().Rotation = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
}

void BindSkyLightProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& sky = system.GetSkyLight();
    AddFloatProperty(editor, "Intensity", "Sky Light", sky.Intensity, [&system](float value) {
        system.GetSkyLight().Intensity = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddBoolProperty(editor, "Real Time Capture", "Sky Light", sky.RealTimeCapture, [&system](bool value) {
        system.GetSkyLight().RealTimeCapture = value;
        system.UpdateRendering();
    });
}

void BindAtmosphereProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& atmosphere = system.GetSkyAtmosphere();
    AddFloatProperty(editor, "Rayleigh Scattering", "Atmosphere", atmosphere.RayleighScattering, [&system](float value) {
        system.GetSkyAtmosphere().RayleighScattering = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Mie Scattering", "Atmosphere", atmosphere.MieScattering, [&system](float value) {
        system.GetSkyAtmosphere().MieScattering = value;
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Mie Anisotropy", "Atmosphere", atmosphere.MieAnisotropy, [&system](float value) {
        system.GetSkyAtmosphere().MieAnisotropy = value;
        system.UpdateRendering();
    });
}

void BindFogProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& fog = system.GetHeightFog();
    AddFloatProperty(editor, "Density", "Fog", fog.Density, [&system](float value) {
        system.GetHeightFog().Density = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Height Falloff", "Fog", fog.HeightFalloff, [&system](float value) {
        system.GetHeightFog().HeightFalloff = value;
        system.UpdateRendering();
    });
    AddBoolProperty(editor, "Volumetric Fog", "Fog", fog.VolumetricFog, [&system](bool value) {
        system.SetVolumetricFogEnabled(value);
    });
    AddVec3Property(editor, "Fog Color", "Fog", fog.FogColor, [&system](const glm::vec3& value) {
        system.GetHeightFog().FogColor = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
}

void BindCloudProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& clouds = system.GetVolumetricClouds();
    AddBoolProperty(editor, "Enabled", "Clouds", clouds.Enabled, [&system](bool value) {
        system.SetVolumetricCloudsEnabled(value);
    });
    AddFloatProperty(editor, "Coverage", "Clouds", clouds.Coverage, [&system](float value) {
        system.GetVolumetricClouds().Coverage = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Altitude", "Clouds", clouds.Altitude, [&system](float value) {
        system.GetVolumetricClouds().Altitude = value;
        system.SyncToScene();
        system.UpdateRendering();
    });
}

void BindExposureProperties(we::UI::PropertyEditor& editor, EnvironmentSystem& system) {
    auto& exposure = system.GetExposureController();
    AddBoolProperty(editor, "Auto Exposure", "Exposure", exposure.AutoExposure, [&system](bool value) {
        system.GetExposureController().AutoExposure = value;
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Exposure EV", "Exposure", exposure.ExposureEV, [&system](float value) {
        system.GetExposureController().ExposureEV = value;
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Exposure Compensation", "Exposure", exposure.ExposureCompensation, [&system](float value) {
        system.GetExposureController().ExposureCompensation = value;
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Min EV", "Exposure", exposure.MinEV, [&system](float value) {
        system.GetExposureController().MinEV = value;
        system.UpdateRendering();
    });
    AddFloatProperty(editor, "Max EV", "Exposure", exposure.MaxEV, [&system](float value) {
        system.GetExposureController().MaxEV = value;
        system.UpdateRendering();
    });
}

void RefreshDetailsPanel() {
    auto scene = g_Scene.lock();
    auto details = g_Details.lock();
    if (!scene || !details) {
        return;
    }

    const int selectedIndex = scene->GetSelectedEntityIndex();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(scene->GetEntities().size())) {
        details->Clear();
        g_LastSelectedEntityId = 0;
        return;
    }

    const Entity& entity = scene->GetEntities()[static_cast<size_t>(selectedIndex)];
    if (entity.Id == g_LastSelectedEntityId) {
        return;
    }
    g_LastSelectedEntityId = entity.Id;

    details->Clear();
    EnvironmentSystem& system = EnvironmentSystem::Get();
    system.SyncFromScene();

    we::UI::Property actorName;
    actorName.name = "Name";
    actorName.category = "Actor";
    actorName.type = we::UI::PropertyType::String;
    actorName.value = entity.Name;
    details->AddProperty(actorName);

    we::UI::Property actorType;
    actorType.name = "Type";
    actorType.category = "Actor";
    actorType.type = we::UI::PropertyType::String;
    actorType.value = EntityTypeLabel(entity.Type);
    details->AddProperty(actorType);

    AddVec3Property(*details, "Position", "Transform", entity.Position, [scene, entityId = entity.Id](const glm::vec3& value) {
        if (Entity* target = scene->FindEntityById(entityId)) {
            target->Position = value;
            EnvironmentSystem::Get().SyncFromScene();
            EnvironmentSystem::Get().UpdateRendering();
        }
    });
    AddVec3Property(*details, "Rotation", "Transform", entity.Rotation, [scene, entityId = entity.Id](const glm::vec3& value) {
        if (Entity* target = scene->FindEntityById(entityId)) {
            target->Rotation = value;
            EnvironmentSystem::Get().SyncFromScene();
            EnvironmentSystem::Get().UpdateRendering();
        }
    });
    AddVec3Property(*details, "Scale", "Transform", entity.Scale, [scene, entityId = entity.Id](const glm::vec3& value) {
        if (Entity* target = scene->FindEntityById(entityId)) {
            target->Scale = value;
            EnvironmentSystem::Get().SyncFromScene();
            EnvironmentSystem::Get().UpdateRendering();
        }
    });

    switch (system.GetActorKind(entity.Id)) {
    case EnvironmentActorKind::DirectionalLight:
        BindSunProperties(*details, system);
        break;
    case EnvironmentActorKind::SkyLight:
        BindSkyLightProperties(*details, system);
        break;
    case EnvironmentActorKind::SkyAtmosphere:
        BindAtmosphereProperties(*details, system);
        break;
    case EnvironmentActorKind::HeightFog:
        BindFogProperties(*details, system);
        break;
    case EnvironmentActorKind::VolumetricClouds:
        BindCloudProperties(*details, system);
        break;
    case EnvironmentActorKind::ExposureController:
        BindExposureProperties(*details, system);
        break;
    default:
        break;
    }
}

std::shared_ptr<we::UI::TreeNode> BuildNodeForEntity(const Entity& entity) {
    auto node = std::make_shared<we::UI::TreeNode>();
    node->id = std::to_string(entity.Id);
    node->label = entity.Name;
    node->iconName = IconForEntity(entity);
    node->userData = reinterpret_cast<void*>(entity.Id);
    if (entity.Type == EntityType::EmptyActor && entity.Name == we::runtime::world::environment::kEnvironmentFolderName) {
        node->expanded = true;
    }
    return node;
}

void RefreshOutliner() {
    auto scene = g_Scene.lock();
    auto outliner = g_Outliner.lock();
    if (!scene || !outliner) {
        return;
    }

    auto root = std::make_shared<we::UI::TreeNode>();
    root->id = "root";
    root->label = "";
    root->expanded = true;

    std::unordered_map<std::uint64_t, std::shared_ptr<we::UI::TreeNode>> nodeById;
    for (const Entity& entity : scene->GetEntities()) {
        nodeById[entity.Id] = BuildNodeForEntity(entity);
    }

    for (const Entity& entity : scene->GetEntities()) {
        auto node = nodeById[entity.Id];
        if (entity.ParentId != 0 && nodeById.contains(entity.ParentId)) {
            nodeById[entity.ParentId]->children.push_back(node);
        } else {
            root->children.push_back(node);
        }
    }

    SortTreeChildren(root->children, *scene);

    outliner->SetRoot(root);

    const int selectedIndex = scene->GetSelectedEntityIndex();
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(scene->GetEntities().size())) {
        outliner->SetSelectedId(std::to_string(scene->GetEntities()[static_cast<size_t>(selectedIndex)].Id));
    }
}

class EnvironmentDropdownMenu : public we::UI::Widget {
public:
    explicit EnvironmentDropdownMenu(std::vector<std::shared_ptr<we::UI::MenuItem>> items)
        : m_Items(std::move(items)) {}

    we::UI::Size Measure(const we::UI::Size& availableSize) override {
        float maxWidth = 220.0f;
        for (const auto& item : m_Items) {
            maxWidth = std::max(maxWidth, 24.0f + item->label.length() * 7.0f + 40.0f);
        }
        m_DesiredSize = we::UI::Size{ maxWidth, 8.0f + m_Items.size() * 28.0f };
        return m_DesiredSize;
    }

    void Arrange(const we::UI::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::UI::PaintContext& context) override {
        context.DrawShadow(m_Geometry, we::UI::Color{ 0.0f, 0.0f, 0.0f, 0.15f }, 4.0f, 12.0f);
        context.DrawRoundedRect(m_Geometry, we::UI::Theme::Get().PanelBackground, 4.0f);
        context.DrawRoundedRectOutline(m_Geometry, we::UI::Theme::Get().BorderDefault, 1.0f, 4.0f);

        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            we::UI::Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (!item->enabled) {
                y += 28.0f;
                continue;
            }
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRoundedRect(row, we::UI::Theme::Get().HoverOverlay, 3.0f);
            }
            context.DrawText(item->label, we::UI::Point{ row.x + 12.0f, row.y + 6.0f },
                we::UI::Theme::Get().TextPrimary, 11.0f);
            if (item->checked) {
                we::UI::IconPainter::DrawIcon(context, we::UI::Icons::CheckName,
                    we::UI::Rect{ row.x + row.width - 22.0f, row.y + 5.0f, 16.0f, 16.0f },
                    we::UI::Theme::Get().SelectedAccent);
            }
            y += 28.0f;
        }
    }

    void OnMouseMove(const we::UI::MouseEvent& event) override {
        m_Hovered = -1;
        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::UI::Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += 28.0f;
        }
    }

    void OnMouseDown(const we::UI::MouseEvent& event) override {
        if (event.button != we::UI::MouseButton::Left) {
            return;
        }

        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::UI::Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (row.Contains(event.position) && m_Items[i]->enabled && m_Items[i]->onClick) {
                m_Items[i]->onClick();
                we::UI::OverlayManager::Get()->CloseAllPopups();
                return;
            }
            y += 28.0f;
        }
    }

private:
    std::vector<std::shared_ptr<we::UI::MenuItem>> m_Items;
    int m_Hovered = -1;
};

class EnvironmentToolbarButton : public we::UI::Widget {
public:
    EnvironmentToolbarButton() = default;

    we::UI::Size Measure(const we::UI::Size& availableSize) override {
        (void)availableSize;
        m_DesiredSize = we::UI::Size{ 102.0f, 22.0f }; // Slightly wider for 18px icon
        return m_DesiredSize;
    }

    void Arrange(const we::UI::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::UI::PaintContext& context) override {
        we::UI::Rect rect = m_Geometry;
        if (m_Hovered || m_Pressed) {
            context.DrawRoundedRect(rect, we::UI::Theme::Get().HoverOverlay, 3.0f);
        }
        
        float centerY = rect.y + rect.height / 2.0f;
        
        we::UI::IconPainter::DrawIcon(context, we::UI::Icons::SunName,
            we::UI::Rect{ rect.x + 6.0f, std::floor(centerY - 9.0f), 18.0f, 18.0f }, we::UI::Theme::Get().TextPrimary);
            
        context.DrawText("Environment", we::UI::Point{ rect.x + 28.0f, std::floor(centerY - 5.5f) },
            we::UI::Theme::Get().TextPrimary, 11.0f, true);
            
        we::UI::IconPainter::DrawIcon(context, we::UI::Icons::ChevronDownName,
            we::UI::Rect{ rect.x + rect.width - 16.0f, std::floor(centerY - 5.0f), 10.0f, 10.0f },
            we::UI::Theme::Get().TextSecondary);
    }

    void OnMouseMove(const we::UI::MouseEvent& event) override {
        m_Hovered = m_Geometry.Contains(event.position);
    }

    void OnMouseDown(const we::UI::MouseEvent& event) override {
        if (event.button != we::UI::MouseButton::Left) {
            return;
        }
        m_Pressed = true;
        ShowMenu();
    }

    void OnMouseUp(const we::UI::MouseEvent& event) override {
        (void)event;
        m_Pressed = false;
    }

    bool ShowsPointerCursor(const we::UI::Point& position) const override {
        return m_Geometry.Contains(position);
    }

private:
    void ShowMenu() {
        auto makeItem = [](const std::string& label, std::function<void()> onClick, bool checked = false, bool enabled = true) {
            auto item = std::make_shared<we::UI::MenuItem>();
            item->label = label;
            item->onClick = std::move(onClick);
            item->checked = checked;
            item->enabled = enabled;
            return item;
        };

        EnvironmentSystem& system = EnvironmentSystem::Get();
        std::vector<std::shared_ptr<we::UI::MenuItem>> items;
        items.push_back(makeItem("Create Environment", []() { EnvironmentSystem::Get().CreateEnvironment(); }));
        items.push_back(makeItem("Reset Environment", []() { EnvironmentSystem::Get().ResetEnvironment(); }));
        items.push_back(makeItem("Remove Environment", []() { EnvironmentSystem::Get().RemoveEnvironment(); }));
        items.push_back(makeItem("Rebuild Environment", []() { EnvironmentSystem::Get().RebuildEnvironment(); }));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Volumetric Fog", []() {
            EnvironmentSystem& env = EnvironmentSystem::Get();
            env.SetVolumetricFogEnabled(!env.IsVolumetricFogEnabled());
        }, system.IsVolumetricFogEnabled()));
        items.push_back(makeItem("Volumetric Clouds", []() {
            EnvironmentSystem& env = EnvironmentSystem::Get();
            env.SetVolumetricCloudsEnabled(!env.IsVolumetricCloudsEnabled());
        }, system.IsVolumetricCloudsEnabled()));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Preset: Sunny", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunny); }));
        items.push_back(makeItem("Preset: Sunset", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunset); }));
        items.push_back(makeItem("Preset: Night", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Night); }));
        items.push_back(makeItem("Preset: Overcast", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Overcast); }));
        items.push_back(makeItem("Preset: Foggy", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Foggy); }));
        items.push_back(makeItem("Preset: Studio", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Studio); }));

        auto menu = std::make_shared<EnvironmentDropdownMenu>(items);
        auto overlay = we::UI::OverlayManager::Get();
        if (!overlay) {
            return;
        }
        overlay->CloseAllPopups();
        overlay->ShowPopup(menu, we::UI::Point{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f });
    }

    bool m_Hovered = false;
    bool m_Pressed = false;
};

} // namespace

void InitializeEditor(
    const std::shared_ptr<Scene>& scene,
    const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer,
    const std::shared_ptr<we::UI::TreeView>& outliner,
    const std::shared_ptr<we::UI::PropertyEditor>& details) {

    g_Scene = scene;
    g_Outliner = outliner;
    g_Details = details;

    EnvironmentSystem::Get().BindScene(scene);
    EnvironmentSystem::Get().BindRenderer(renderer);
    EnvironmentSystem::Get().AddChangeListener([]() {
        RefreshOutliner();
        RefreshDetailsPanel();
    });

    if (outliner) {
        outliner->SetOnSelectionChanged([scene](const std::vector<std::string>& ids) {
            if (ids.empty()) {
                return;
            }
            const std::uint64_t entityId = std::strtoull(ids.back().c_str(), nullptr, 10);
            if (entityId == 0) {
                return;
            }
            if (const int index = scene->FindEntityIndexById(entityId); index >= 0) {
                scene->SetSelectedEntityIndex(index);
                g_LastSelectedEntityId = 0;
                RefreshDetailsPanel();
            }
        });

        outliner->SetOnRenameCommitted([scene](const std::string& id, const std::string& newLabel) {
            const std::uint64_t entityId = std::strtoull(id.c_str(), nullptr, 10);
            if (entityId == 0) {
                return;
            }
            if (Entity* entity = scene->FindEntityById(entityId)) {
                entity->Name = newLabel;
                RefreshOutliner();
                RefreshDetailsPanel();
            }
        });

        outliner->SetOnReparentRequested([scene](const std::string& childId, const std::string& newParentId) {
            const std::uint64_t childEntityId = std::strtoull(childId.c_str(), nullptr, 10);
            const std::uint64_t parentEntityId = std::strtoull(newParentId.c_str(), nullptr, 10);
            if (childEntityId == 0 || parentEntityId == 0 || childEntityId == parentEntityId) {
                return;
            }
            if (Entity* child = scene->FindEntityById(childEntityId)) {
                child->ParentId = parentEntityId;
                RefreshOutliner();
            }
        });
    }

    RefreshOutliner();
    RefreshDetailsPanel();
}

std::shared_ptr<we::UI::Widget> CreateEnvironmentToolbarMenu() {
    return std::make_shared<EnvironmentToolbarButton>();
}

void TickEditor() {
    auto scene = g_Scene.lock();
    auto outliner = g_Outliner.lock();
    if (!scene) {
        return;
    }

    const int selectedIndex = scene->GetSelectedEntityIndex();
    std::uint64_t selectedId = 0;
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(scene->GetEntities().size())) {
        selectedId = scene->GetEntities()[static_cast<size_t>(selectedIndex)].Id;
    }

    if (outliner && selectedId != 0) {
        const std::string selectedNodeId = std::to_string(selectedId);
        if (outliner->GetSelectedId() != selectedNodeId) {
            outliner->SetSelectedId(selectedNodeId);
        }
    }

    if (selectedId != g_LastSelectedEntityId) {
        RefreshDetailsPanel();
    }
}

} // namespace we::editor::environment
