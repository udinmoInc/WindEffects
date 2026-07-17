#include "Environment/EnvironmentEditorApi.h"

#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentTypes.h"
#include "Environment/EnvironmentTypes.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Widgets/PropertyEditor.h"
#include "Widgets/TreeView.h"
#include "Widgets/MenuBar.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"

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
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


namespace {

using we::runtime::scene::Entity;
using we::runtime::scene::EntityType;
using we::runtime::scene::Scene;
using we::runtime::world::environment::EnvironmentActorKind;
using we::runtime::world::environment::EnvironmentPreset;
using we::runtime::world::environment::EnvironmentSystem;

std::weak_ptr<Scene> g_Scene;
std::weak_ptr<::we::editor::contentbrowser::TreeView> g_Outliner;
std::weak_ptr<::we::editor::property::PropertyEditor> g_Details;
std::uint64_t g_LastSelectedEntityId = 0;

std::string EntityTypeLabel(EntityType type) {
    switch (type) {
    case EntityType::DirectionalLight: return "Directional Light";
    case EntityType::SkyLight: return "Sky Light";
    case EntityType::SkyAtmosphere: return "Sky Atmosphere";
    case EntityType::HeightFog: return "Exponential Height Fog";
    case EntityType::VolumetricClouds: return "Cloud";
    case EntityType::EmptyActor: return "Folder";
    default: return "Actor";
    }
}

std::string IconForEntity(const Entity& entity) {
    switch (entity.Type) {
    case EntityType::DirectionalLight:
        return we::runtime::kindui::Icons::SunName;
    case EntityType::SkyLight:
        return we::runtime::kindui::Icons::LightName;
    case EntityType::SkyAtmosphere:
        return we::runtime::kindui::Icons::SphereName;
    case EntityType::HeightFog:
        return we::runtime::kindui::Icons::LayersName;
    case EntityType::VolumetricClouds:
        return we::runtime::kindui::Icons::SphereName;
    case EntityType::EmptyActor:
        return we::runtime::kindui::Icons::FolderName;
    default:
        return we::runtime::kindui::Icons::CubeName;
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

void SortTreeChildren(std::vector<std::shared_ptr<::we::editor::contentbrowser::TreeNode>>& children, Scene& scene) {
    std::sort(children.begin(), children.end(), [&](const std::shared_ptr<::we::editor::contentbrowser::TreeNode>& a, const std::shared_ptr<::we::editor::contentbrowser::TreeNode>& b) {
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
    ::we::editor::property::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    bool value,
    std::function<void(bool)> onChanged) {
    ::we::editor::property::Property property;
    property.name = name;
    property.category = category;
    property.type = ::we::editor::property::PropertyType::Bool;
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
    ::we::editor::property::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    float value,
    std::function<void(float)> onChanged) {
    ::we::editor::property::Property property;
    property.name = name;
    property.category = category;
    property.type = ::we::editor::property::PropertyType::Float;
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
    ::we::editor::property::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    int value,
    std::function<void(int)> onChanged) {
    ::we::editor::property::Property property;
    property.name = name;
    property.category = category;
    property.type = ::we::editor::property::PropertyType::Int;
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
    ::we::editor::property::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    const glm::vec3& value,
    std::function<void(const glm::vec3&)> onChanged) {
    ::we::editor::property::Property property;
    property.name = name;
    property.category = category;
    property.type = ::we::editor::property::PropertyType::Vector3;
    property.value = FormatVec3(value);
    property.defaultValue = property.value;
    property.onValueChanged = [value, onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(ParseVec3(newValue, value));
        }
    };
    editor.AddProperty(property);
}

void BindSunProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
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

void BindSkyLightProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
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

void BindAtmosphereProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
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

void BindFogProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
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

void AddStringProperty(
    ::we::editor::property::PropertyEditor& editor,
    const std::string& name,
    const std::string& category,
    const std::string& value,
    std::function<void(const std::string&)> onChanged) {
    ::we::editor::property::Property property;
    property.name = name;
    property.category = category;
    property.type = ::we::editor::property::PropertyType::String;
    property.value = value;
    property.defaultValue = property.value;
    property.onValueChanged = [onChanged](const std::string& newValue) {
        if (onChanged) {
            onChanged(newValue);
        }
    };
    editor.AddProperty(property);
}

void BindCloudProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
    using we::runtime::world::environment::CloudPreset;
    using we::runtime::world::environment::CloudQualityPreset;
    using we::runtime::world::environment::EnvironmentVolumetricClouds;

    auto& clouds = system.GetVolumetricClouds();
    auto refresh = [&system]() {
        system.GetVolumetricClouds().SyncAltitudeFromBounds();
        system.SyncToScene();
        system.UpdateRendering();
    };

    AddBoolProperty(editor, "Enabled", "Clouds", clouds.Enabled, [&system](bool value) {
        system.SetVolumetricCloudsEnabled(value);
    });
    AddStringProperty(editor, "Preset", "Clouds", EnvironmentVolumetricClouds::PresetName(clouds.ActivePreset),
        [&system](const std::string& value) {
            for (int i = 0; i <= static_cast<int>(CloudPreset::Stratocumulus); ++i) {
                const auto preset = static_cast<CloudPreset>(i);
                if (value == EnvironmentVolumetricClouds::PresetName(preset)) {
                    system.ApplyCloudPreset(preset);
                    return;
                }
            }
        });
    AddStringProperty(editor, "Quality Preset", "Clouds",
        clouds.Quality == CloudQualityPreset::Low ? "Low"
            : clouds.Quality == CloudQualityPreset::High ? "High"
            : clouds.Quality == CloudQualityPreset::Epic ? "Epic" : "Medium",
        [&system, refresh](const std::string& value) {
            auto& c = system.GetVolumetricClouds();
            if (value == "Low") c.Quality = CloudQualityPreset::Low;
            else if (value == "High") c.Quality = CloudQualityPreset::High;
            else if (value == "Epic") c.Quality = CloudQualityPreset::Epic;
            else c.Quality = CloudQualityPreset::Medium;
            refresh();
        });

    AddFloatProperty(editor, "Coverage", "Clouds", clouds.Coverage, [&system, refresh](float value) {
        system.GetVolumetricClouds().Coverage = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
    AddFloatProperty(editor, "Density", "Clouds", clouds.Density, [&system, refresh](float value) {
        system.GetVolumetricClouds().Density = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Density Multiplier", "Clouds", clouds.DensityMultiplier, [&system, refresh](float value) {
        system.GetVolumetricClouds().DensityMultiplier = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Cloud Height", "Clouds", clouds.CloudHeight, [&system, refresh](float value) {
        auto& c = system.GetVolumetricClouds();
        const float half = c.CloudThickness * 0.5f;
        c.CloudHeight = std::max(0.0f, value);
        c.BottomAltitude = c.CloudHeight - half;
        c.TopAltitude = c.CloudHeight + half;
        refresh();
    });
    AddFloatProperty(editor, "Cloud Thickness", "Clouds", clouds.CloudThickness, [&system, refresh](float value) {
        auto& c = system.GetVolumetricClouds();
        c.CloudThickness = std::max(50.0f, value);
        c.BottomAltitude = c.CloudHeight - c.CloudThickness * 0.5f;
        c.TopAltitude = c.CloudHeight + c.CloudThickness * 0.5f;
        refresh();
    });
    AddFloatProperty(editor, "Bottom Altitude", "Clouds", clouds.BottomAltitude, [&system, refresh](float value) {
        system.GetVolumetricClouds().BottomAltitude = value;
        refresh();
    });
    AddFloatProperty(editor, "Top Altitude", "Clouds", clouds.TopAltitude, [&system, refresh](float value) {
        system.GetVolumetricClouds().TopAltitude = value;
        refresh();
    });
    AddVec3Property(editor, "Wind Direction", "Clouds", clouds.WindDirection, [&system, refresh](const glm::vec3& value) {
        system.GetVolumetricClouds().WindDirection = value;
        refresh();
    });
    AddFloatProperty(editor, "Wind Speed", "Clouds", clouds.WindSpeed, [&system, refresh](float value) {
        system.GetVolumetricClouds().WindSpeed = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Animation Speed", "Clouds", clouds.AnimationSpeed, [&system, refresh](float value) {
        system.GetVolumetricClouds().AnimationSpeed = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Noise Scale", "Clouds", clouds.NoiseScale, [&system, refresh](float value) {
        system.GetVolumetricClouds().NoiseScale = std::max(0.05f, value);
        refresh();
    });
    AddFloatProperty(editor, "Detail Noise Scale", "Clouds", clouds.DetailNoiseScale, [&system, refresh](float value) {
        system.GetVolumetricClouds().DetailNoiseScale = std::max(0.5f, value);
        refresh();
    });
    AddFloatProperty(editor, "Shape Noise", "Clouds", clouds.ShapeNoise, [&system, refresh](float value) {
        system.GetVolumetricClouds().ShapeNoise = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
    AddFloatProperty(editor, "Erosion Noise", "Clouds", clouds.ErosionNoise, [&system, refresh](float value) {
        system.GetVolumetricClouds().ErosionNoise = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
    AddFloatProperty(editor, "Seed", "Clouds", clouds.Seed, [&system, refresh](float value) {
        system.GetVolumetricClouds().Seed = value;
        refresh();
    });
    AddFloatProperty(editor, "Lighting Intensity", "Lighting", clouds.LightingIntensity, [&system, refresh](float value) {
        system.GetVolumetricClouds().LightingIntensity = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Silver Lining Intensity", "Lighting", clouds.SilverLiningIntensity, [&system, refresh](float value) {
        system.GetVolumetricClouds().SilverLiningIntensity = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Ambient Contribution", "Lighting", clouds.AmbientContribution, [&system, refresh](float value) {
        system.GetVolumetricClouds().AmbientContribution = std::max(0.0f, value);
        refresh();
    });
    AddFloatProperty(editor, "Multi-Scattering Strength", "Lighting", clouds.MultiScatteringStrength, [&system, refresh](float value) {
        system.GetVolumetricClouds().MultiScatteringStrength = std::clamp(value, 0.0f, 2.0f);
        refresh();
    });
    AddFloatProperty(editor, "Phase Function (Forward Scattering)", "Lighting", clouds.PhaseG, [&system, refresh](float value) {
        system.GetVolumetricClouds().PhaseG = std::clamp(value, 0.0f, 0.95f);
        refresh();
    });
    AddFloatProperty(editor, "Powder Effect", "Lighting", clouds.PowderEffect, [&system, refresh](float value) {
        system.GetVolumetricClouds().PowderEffect = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
    AddFloatProperty(editor, "Shadow Strength", "Shadows", clouds.ShadowStrength, [&system, refresh](float value) {
        system.GetVolumetricClouds().ShadowStrength = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
    AddFloatProperty(editor, "Shadow Distance", "Shadows", clouds.ShadowDistance, [&system, refresh](float value) {
        system.GetVolumetricClouds().ShadowDistance = std::max(100.0f, value);
        refresh();
    });
    AddIntProperty(editor, "Shadow Resolution", "Shadows", clouds.ShadowResolution, [&system, refresh](int value) {
        system.GetVolumetricClouds().ShadowResolution = std::clamp(value, 64, 2048);
        refresh();
    });
    AddVec3Property(editor, "Cloud Color Tint", "Appearance", clouds.CloudColorTint, [&system, refresh](const glm::vec3& value) {
        system.GetVolumetricClouds().CloudColorTint = value;
        refresh();
    });
    AddVec3Property(editor, "Cloud Color", "Appearance", clouds.CloudColor, [&system, refresh](const glm::vec3& value) {
        system.GetVolumetricClouds().CloudColor = value;
        refresh();
    });
    AddFloatProperty(editor, "Weather Map Influence", "Weather", clouds.WeatherMapInfluence, [&system, refresh](float value) {
        system.GetVolumetricClouds().WeatherMapInfluence = std::clamp(value, 0.0f, 1.0f);
        refresh();
    });
}

void BindExposureProperties(::we::editor::property::PropertyEditor& editor, EnvironmentSystem& system) {
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

    ::we::editor::property::Property actorName;
    actorName.name = "Name";
    actorName.category = "Actor";
    actorName.type = ::we::editor::property::PropertyType::String;
    actorName.value = entity.Name;
    details->AddProperty(actorName);

    ::we::editor::property::Property actorType;
    actorType.name = "Type";
    actorType.category = "Actor";
    actorType.type = ::we::editor::property::PropertyType::String;
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

std::shared_ptr<::we::editor::contentbrowser::TreeNode> BuildNodeForEntity(const Entity& entity) {
    auto node = std::make_shared<::we::editor::contentbrowser::TreeNode>();
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

    auto root = std::make_shared<::we::editor::contentbrowser::TreeNode>();
    root->id = "root";
    root->label = "";
    root->expanded = true;

    std::unordered_map<std::uint64_t, std::shared_ptr<::we::editor::contentbrowser::TreeNode>> nodeById;
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

class EnvironmentDropdownMenu : public we::runtime::kindui::Widget {
public:
    explicit EnvironmentDropdownMenu(std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items)
        : m_Items(std::move(items)) {}

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override {
        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        const float textSize = ThemeMetric(we::runtime::kindui::MetricToken::TextSizeSmall);
        float maxWidth = 200.0f;
        for (const auto& item : m_Items) {
            maxWidth = std::max(maxWidth, ThemeMetric(we::runtime::kindui::MetricToken::Space6) + item->label.length() * textSize * 0.52f + 28.0f);
        }
        m_DesiredSize = we::runtime::kindui::Size{ maxWidth, padY * 2.0f + m_Items.size() * rowH };
        return m_DesiredSize;
    }

    void Arrange(const we::runtime::kindui::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::runtime::kindui::PaintContext& context) override {
        context.DrawShadow(m_Geometry, ThemeColor(we::runtime::kindui::ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_Geometry, ThemeColor(we::runtime::kindui::ColorToken::PopupBackground), ThemeMetric(we::runtime::kindui::MetricToken::CornerRadiusSmall));

        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        const float padX = ThemeMetric(we::runtime::kindui::MetricToken::Space2);
        const float textSize = ThemeMetric(we::runtime::kindui::MetricToken::TextSizeSmall);
        const float iconSize = ThemeMetric(we::runtime::kindui::MetricToken::IconSizeTree);

        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (!item->enabled) {
                y += rowH;
                continue;
            }
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRect(row, ThemeColor(we::runtime::kindui::ColorToken::HoverBackground));
            }
            context.DrawText(item->label, we::runtime::kindui::Point{ row.x + padX, row.y + (rowH - textSize) * 0.5f },
                ThemeColor(we::runtime::kindui::ColorToken::TextPrimary), textSize);
            if (item->checked) {
                we::runtime::kindui::IconPainter::DrawIcon(context, we::runtime::kindui::Icons::CheckName,
                    we::runtime::kindui::Rect{ row.x + row.width - padX - iconSize, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize },
                    ThemeColor(we::runtime::kindui::ColorToken::AccentPrimary));
            }
            y += rowH;
        }
    }

    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override {
        m_Hovered = -1;
        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += rowH;
        }
    }

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override {
        if (event.button != we::runtime::kindui::MouseButton::Left) {
            return;
        }

        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position) && m_Items[i]->enabled && m_Items[i]->onClick) {
                m_Items[i]->onClick();
                we::programs::editor::GetEditorPopupHost()->CloseAllPopups();
                return;
            }
            y += rowH;
        }
    }

private:
    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> m_Items;
    int m_Hovered = -1;
};

class EnvironmentToolbarButton : public we::runtime::kindui::Widget {
public:
    EnvironmentToolbarButton() = default;

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override {
        (void)availableSize;
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        const float padH = we::runtime::kindui::ToolbarButtonChrome::HorizontalPad(uiScale);
        const float iconSz = we::runtime::kindui::ToolbarButtonChrome::IconSize(uiScale);
        const float iconGap = we::runtime::kindui::ToolbarButtonChrome::IconGapPx(uiScale);
        const float chevW = we::runtime::kindui::IconMetrics::CompactDisplayPx();
        const float controlH = ThemeMetric(we::runtime::kindui::MetricToken::IconButtonSize) * uiScale;
        m_DesiredSize = we::runtime::kindui::Size{
            padH + iconSz + iconGap + chevW + padH,
            controlH
        };
        return m_DesiredSize;
    }

    void Arrange(const we::runtime::kindui::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::runtime::kindui::PaintContext& context) override {
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        m_HoverAnim = we::runtime::kindui::Animator::Damp(
            m_HoverAnim, m_Hovered ? 1.0f : 0.0f,
            ThemeMetric(we::runtime::kindui::MetricToken::HoverAnimationDamping));

        const float pressStrength = m_Pressed ? 1.0f : 0.0f;
        we::runtime::kindui::ToolbarButtonChrome::PaintIconButton(
            context, m_Geometry, m_HoverAnim, pressStrength, false, 0.0f, uiScale);

        const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
        const float padH = we::runtime::kindui::ToolbarButtonChrome::HorizontalPad(uiScale);
        const float iconSize = we::runtime::kindui::ToolbarButtonChrome::IconSize(uiScale);
        we::runtime::kindui::Color iconColor = we::runtime::kindui::ToolbarButtonChrome::ResolveIconColor(
            m_HoverAnim, pressStrength, false);

        we::runtime::kindui::IconPainter::DrawIcon(context, we::runtime::kindui::Icons::ToolbarEnvironmentName,
            we::runtime::kindui::ToolbarButtonChrome::PlaceIconInControl(
                we::runtime::kindui::Rect{ m_Geometry.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize },
                iconSize),
            iconColor);

        const float display = we::runtime::kindui::IconMetrics::CompactDisplayPx();
        const float chevronX = m_Geometry.x + m_Geometry.width - padH - display;
        we::runtime::kindui::Rect chevronControl{ chevronX, centerY - display * 0.5f, display, display };
        we::runtime::kindui::IconPainter::DrawCompactIcon(context, we::runtime::kindui::Icons::ChevronDownName,
            chevronControl, iconColor);
    }

    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override {
        m_Hovered = m_Geometry.Contains(event.position);
    }

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override {
        if (event.button != we::runtime::kindui::MouseButton::Left) {
            return;
        }
        m_Pressed = true;
        ShowMenu();
    }

    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override {
        (void)event;
        m_Pressed = false;
    }

    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override {
        return m_Geometry.Contains(position);
    }

private:
    void ShowMenu() {
        auto makeItem = [](const std::string& label, std::function<void()> onClick, bool checked = false, bool enabled = true) {
            auto item = std::make_shared<::we::editor::menus::MenuItem>();
            item->label = label;
            item->onClick = std::move(onClick);
            item->checked = checked;
            item->enabled = enabled;
            return item;
        };

        EnvironmentSystem& system = EnvironmentSystem::Get();
        std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
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
        items.push_back(makeItem("Cloud: Clear Sky", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::ClearSky); }));
        items.push_back(makeItem("Cloud: Few Clouds", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::FewClouds); }));
        items.push_back(makeItem("Cloud: Scattered", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::ScatteredClouds); }));
        items.push_back(makeItem("Cloud: Broken", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::BrokenClouds); }));
        items.push_back(makeItem("Cloud: Overcast", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Overcast); }));
        items.push_back(makeItem("Cloud: Storm", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Storm); }));
        items.push_back(makeItem("Cloud: Heavy Storm", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::HeavyStorm); }));
        items.push_back(makeItem("Cloud: Sunset", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::SunsetClouds); }));
        items.push_back(makeItem("Cloud: Sunrise", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::SunriseClouds); }));
        items.push_back(makeItem("Cloud: High Cirrus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::HighCirrus); }));
        items.push_back(makeItem("Cloud: Cumulus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Cumulus); }));
        items.push_back(makeItem("Cloud: Stratocumulus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Stratocumulus); }));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Preset: Sunny", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunny); }));
        items.push_back(makeItem("Preset: Sunset", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunset); }));
        items.push_back(makeItem("Preset: Night", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Night); }));
        items.push_back(makeItem("Preset: Overcast", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Overcast); }));
        items.push_back(makeItem("Preset: Foggy", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Foggy); }));
        items.push_back(makeItem("Preset: Studio", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Studio); }));

        auto menu = std::make_shared<EnvironmentDropdownMenu>(items);
        auto* overlay = we::programs::editor::GetEditorPopupHost();
        if (!overlay) {
            return;
        }
        overlay->CloseAllPopups();
        overlay->ShowPopup(menu, we::runtime::kindui::Point{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f });
    }

    bool m_Hovered = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
};

} // namespace

void InitializeEditor(
    const std::shared_ptr<Scene>& scene,
    const std::shared_ptr<::we::editor::contentbrowser::TreeView>& outliner,
    const std::shared_ptr<::we::editor::property::PropertyEditor>& details) {

    g_Scene = scene;
    g_Outliner = outliner;
    g_Details = details;

    EnvironmentSystem::Get().BindScene(scene);
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

std::shared_ptr<we::runtime::kindui::Widget> CreateEnvironmentToolbarMenu() {
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
