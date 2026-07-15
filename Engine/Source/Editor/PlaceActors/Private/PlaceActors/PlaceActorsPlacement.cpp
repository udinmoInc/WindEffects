#include "PlaceActors/PlaceActorsPlacement.h"

#include "EditorToolsRegistry.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "TerrainEditor/TerrainEditorService.h"
#include "Core/Logger.h"

#include <glm/gtc/constants.hpp>
#include <unordered_map>

namespace we::programs::editor {

namespace {

using we::runtime::scene::EntityType;

EntityType EntityTypeForTool(const std::string& toolId) {
    static const std::unordered_map<std::string, EntityType> kMap = {
        {"PlaceEmptyActor", EntityType::EmptyActor},
        {"PlaceEmptyCharacter", EntityType::Character},
        {"PlaceEmptyPawn", EntityType::Character},
        {"PlaceCharacter", EntityType::Character},
        {"PlaceBlueprint", EntityType::Blueprint},
        {"PlaceCube", EntityType::Cube},
        {"PlaceSphere", EntityType::Sphere},
        {"PlacePlane", EntityType::Plane},
        {"PlaceCylinder", EntityType::Cylinder},
        {"PlaceCone", EntityType::Cylinder},
        {"PlaceCapsule", EntityType::Sphere},
        {"LightDirectional", EntityType::DirectionalLight},
        {"LightPoint", EntityType::PointLight},
        {"LightSpot", EntityType::SpotLight},
        {"LightSpotAlt", EntityType::SpotLight},
        {"PlaceCamera", EntityType::CameraIcon},
        {"AudioPlace", EntityType::AudioSource},
        {"FXSpawn", EntityType::Volume},
        {"UIWidget", EntityType::Volume},
        {"NavPaint", EntityType::Volume},
        {"PhysicsCollision", EntityType::Volume},
        {"TerrainGenerate", EntityType::Landscape},
    };
    auto it = kMap.find(toolId);
    return it != kMap.end() ? it->second : EntityType::EmptyActor;
}

std::string DefaultNameForTool(const std::string& toolId, const std::string& label) {
    if (!label.empty()) {
        return label;
    }
    return toolId;
}

} // namespace

PlaceActorsPlacement& PlaceActorsPlacement::Get() {
    static PlaceActorsPlacement instance;
    return instance;
}

void PlaceActorsPlacement::BindScene(const std::shared_ptr<we::runtime::scene::Scene>& scene,
                                     const std::shared_ptr<we::runtime::engine::EditorCamera>& camera) {
    m_Scene = scene;
    m_Camera = camera;
    if (scene) {
        we::editor::terrain::TerrainEditorService::Get().BindScene(scene.get());
    }
}

glm::vec3 PlaceActorsPlacement::ComputeSpawnPosition() const {
    auto camera = m_Camera.lock();
    if (!camera) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::vec3 forward = camera->GetForward();
    const glm::vec3 position = camera->GetPosition() + forward * 4.0f;
    return glm::vec3(position.x, std::max(0.0f, position.y), position.z);
}

bool PlaceActorsPlacement::SpawnTool(const std::string& toolId) {
    return SpawnToolAt(toolId, ComputeSpawnPosition());
}

bool PlaceActorsPlacement::SpawnToolAt(const std::string& toolId, const glm::vec3& worldPosition) {
    auto scene = m_Scene.lock();
    if (!scene) {
        HE_ERROR("[PlaceActors] Scene is not bound.");
        return false;
    }

    if (toolId == "TerrainGenerate") {
        we::editor::terrain::TerrainEditorService::Get().BindScene(scene.get());
        if (!we::editor::terrain::TerrainEditorService::Get().GenerateDefaultLandscape()) {
            return false;
        }
        EditorToolsRegistry::Get().RecordToolUsage(toolId);
        HE_INFO("[PlaceActors] Spawned Landscape terrain.");
        return true;
    }

    const EditorToolAction* tool = EditorToolsRegistry::Get().FindTool(toolId);
    const std::string name = DefaultNameForTool(toolId, tool ? tool->label : toolId);
    const EntityType type = EntityTypeForTool(toolId);

    try {
        scene->CreateEntity(name, type);
        auto& entities = scene->GetEntities();
        if (!entities.empty()) {
            entities.back().Position = worldPosition;
            scene->SetSelectedEntityIndex(static_cast<int>(entities.size()) - 1);
        }
        EditorToolsRegistry::Get().RecordToolUsage(toolId);
        if (tool && tool->onExecute) {
            tool->onExecute();
        }
        HE_INFO("[PlaceActors] Spawned '" + name + "' at world position.");
        return true;
    } catch (const std::exception& ex) {
        HE_ERROR(std::string("[PlaceActors] Failed to spawn: ") + ex.what());
        return false;
    }
}

void PlaceActorsPlacement::BeginDragPlacement(const std::string& toolId) {
    m_ActivePlacementToolId = toolId;
}

void PlaceActorsPlacement::CancelDragPlacement() {
    m_ActivePlacementToolId.clear();
}

bool PlaceActorsPlacement::TryPlaceAtViewportPoint(float viewportLocalX, float viewportLocalY) {
    (void)viewportLocalX;
    (void)viewportLocalY;
    if (m_ActivePlacementToolId.empty()) {
        return false;
    }

    const std::string toolId = m_ActivePlacementToolId;
    m_ActivePlacementToolId.clear();
    return SpawnTool(toolId);
}

} // namespace we::programs::editor
