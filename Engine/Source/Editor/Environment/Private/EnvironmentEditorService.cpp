#include "Environment/EnvironmentEditorApi.h"
#include "Core/Math/Types.h"

#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentTypes.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "PropertyEditor/IDetailsView.h"
#include "PropertyEditor/PropertyChangeEvent.h"
#include "PropertyEditor/PropertyEditorTypes.h"
#include "Reflection/TypeId.h"
#include "ContentBrowser/Widgets/TreeView.h"
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
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::Animator;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;
using ::we::runtime::kindui::DPIContext;


namespace {

using we::runtime::scene::Entity;
using we::runtime::scene::EntityType;
using we::runtime::scene::Scene;
using we::runtime::world::environment::EnvironmentActorKind;
using we::runtime::world::environment::EnvironmentPreset;
using we::runtime::world::environment::EnvironmentSystem;

std::weak_ptr<Scene> g_Scene;
std::weak_ptr<::we::editor::contentbrowser::TreeView> g_Outliner;
std::weak_ptr<::we::editor::property::IDetailsView> g_Details;
bool g_DetailsListenerWired = false;
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

void RefreshOutliner();

void OnDetailsPropertyChanged() {
    EnvironmentSystem& system = EnvironmentSystem::Get();
    system.SyncFromScene();
    system.SyncToScene();
    system.UpdateRendering();
    RefreshOutliner();
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

    Entity* entity = scene->FindEntityById(scene->GetEntities()[static_cast<size_t>(selectedIndex)].Id);
    if (!entity) {
        details->Clear();
        g_LastSelectedEntityId = 0;
        return;
    }

    if (entity->Id == g_LastSelectedEntityId) {
        return;
    }
    g_LastSelectedEntityId = entity->Id;

    EnvironmentSystem& system = EnvironmentSystem::Get();
    system.SyncFromScene();

    std::vector<::we::editor::property::ObjectBinding> bindings;
    ::we::editor::property::ObjectBinding entityBinding;
    entityBinding.typeId = we::runtime::reflection::MakeTypeId("we::runtime::scene::Entity");
    entityBinding.instance = entity;
    bindings.push_back(entityBinding);

    switch (system.GetActorKind(entity->Id)) {
    case EnvironmentActorKind::DirectionalLight:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentDirectionalLight"),
            &system.GetSun()});
        break;
    case EnvironmentActorKind::SkyLight:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentSkyLight"),
            &system.GetSkyLight()});
        break;
    case EnvironmentActorKind::SkyAtmosphere:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentSkyAtmosphere"),
            &system.GetSkyAtmosphere()});
        break;
    case EnvironmentActorKind::HeightFog:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentHeightFog"),
            &system.GetHeightFog()});
        break;
    case EnvironmentActorKind::VolumetricClouds:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentVolumetricClouds"),
            &system.GetVolumetricClouds()});
        break;
    case EnvironmentActorKind::ExposureController:
        bindings.push_back({
            we::runtime::reflection::MakeTypeId("we::runtime::world::environment::EnvironmentExposureController"),
            &system.GetExposureController()});
        break;
    default:
        break;
    }

    details->SetBindings(bindings);
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


} // namespace

void InitializeEditor(
    const std::shared_ptr<Scene>& scene,
    const std::shared_ptr<::we::editor::contentbrowser::TreeView>& outliner,
    const std::shared_ptr<::we::editor::property::IDetailsView>& details) {

    g_Scene = scene;
    g_Outliner = outliner;
    g_Details = details;
    g_LastSelectedEntityId = 0;

    if (details && !g_DetailsListenerWired) {
        details->AddChangeListener([](const ::we::editor::property::PropertyChangeEvent&) {
            OnDetailsPropertyChanged();
        });
        g_DetailsListenerWired = true;
    }

    EnvironmentSystem::Get().BindScene(scene);
    EnvironmentSystem::Get().AddChangeListener([]() {
        g_LastSelectedEntityId = 0;
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