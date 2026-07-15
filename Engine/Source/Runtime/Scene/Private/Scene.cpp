#include "Scene/Scene.h"
#include "Core/Logger.h"
#include "ECS/Registry.h"
#include "ECS/System.h"
#include "ECS/Components/CoreComponents.h"
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif
namespace we::runtime::scene {

namespace {

using we::runtime::ecs::NameComponent;
using we::runtime::ecs::TransformComponent;
using we::runtime::ecs::HierarchyComponent;
using we::runtime::ecs::LegacyActorComponent;
using we::runtime::ecs::MaterialComponent;
using we::runtime::ecs::StaticMeshComponent;
using we::runtime::ecs::DirectionalLightComponent;
using we::runtime::ecs::PointLightComponent;
using we::runtime::ecs::SpotLightComponent;
using we::runtime::ecs::CameraComponent;
using we::runtime::ecs::SkyAtmosphereComponent;
using we::runtime::ecs::VolumetricCloudComponent;
using we::runtime::ecs::TerrainComponent;
using we::runtime::ecs::AudioSourceComponent;

void ApplyDefaultEntityProperties(Entity& entity, EntityType type) {
    entity.Mode = 0;

    switch (type) {
    case EntityType::Cube:
        entity.Color = glm::vec4(0.35f, 0.55f, 0.95f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Sphere:
        entity.Color = glm::vec4(0.45f, 0.75f, 0.55f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        entity.Scale = glm::vec3(1.0f);
        break;
    case EntityType::Cylinder:
        entity.Color = glm::vec4(0.85f, 0.55f, 0.35f, 0.85f);
        entity.Position = glm::vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Plane:
        entity.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        entity.Scale = glm::vec3(4.0f, 1.0f, 4.0f);
        break;
    case EntityType::DirectionalLight:
    case EntityType::PointLight:
    case EntityType::SpotLight:
    case EntityType::SkyLight:
    case EntityType::SkyAtmosphere:
    case EntityType::HeightFog:
    case EntityType::VolumetricClouds:
        entity.EditorOnly = true;
        entity.Mode = 1;
        break;
    case EntityType::Landscape:
        entity.Color = glm::vec4(0.35f, 0.55f, 0.28f, 1.0f);
        entity.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        break;
    case EntityType::CameraIcon:
    case EntityType::EmptyActor:
    case EntityType::Character:
    case EntityType::Blueprint:
    case EntityType::AudioSource:
    case EntityType::Volume:
        entity.EditorOnly = true;
        break;
    default:
        break;
    }
}

} // namespace

Scene::Scene()
    : m_Registry(std::make_unique<we::runtime::ecs::Registry>())
    , m_Systems(std::make_unique<we::runtime::ecs::SystemScheduler>()) {
    we::runtime::ecs::RegisterDefaultSystems(*m_Systems);
    m_Systems->OnCreate(*m_Registry);
}

Scene::~Scene() {
    if (m_Systems && m_Registry) {
        m_Systems->OnDestroy(*m_Registry);
    }
    DestroyEntity(0xFFFFFFFF);
}

we::runtime::ecs::Registry& Scene::Registry() {
    return *m_Registry;
}

const we::runtime::ecs::Registry& Scene::Registry() const {
    return *m_Registry;
}

we::runtime::ecs::SystemScheduler& Scene::Systems() {
    return *m_Systems;
}

void Scene::AttachEcsComponents(Entity& entity, std::uint64_t ecsEntityId) {
    const we::runtime::ecs::Entity ecsEntity{ ecsEntityId };
    m_Registry->Replace(ecsEntity, NameComponent{ entity.Name });

    TransformComponent transform{};
    transform.localPosition = entity.Position;
    transform.localRotation = entity.Rotation;
    transform.localScale = entity.Scale;
    transform.dirty = true;
    m_Registry->Replace(ecsEntity, transform);

    m_Registry->Replace(ecsEntity, LegacyActorComponent{
        static_cast<int>(entity.Type),
        entity.EditorOnly,
        entity.Mode
    });

    MaterialComponent material{};
    material.color = entity.Color;
    m_Registry->Replace(ecsEntity, material);

    switch (entity.Type) {
    case EntityType::Cube:
    case EntityType::Sphere:
    case EntityType::Cylinder:
    case EntityType::Plane:
    case EntityType::GroundPlane:
        m_Registry->Replace(ecsEntity, StaticMeshComponent{});
        break;
    case EntityType::DirectionalLight:
        m_Registry->Replace(ecsEntity, DirectionalLightComponent{});
        break;
    case EntityType::PointLight:
        m_Registry->Replace(ecsEntity, PointLightComponent{});
        break;
    case EntityType::SpotLight:
        m_Registry->Replace(ecsEntity, SpotLightComponent{});
        break;
    case EntityType::CameraIcon:
        m_Registry->Replace(ecsEntity, CameraComponent{});
        break;
    case EntityType::SkyAtmosphere:
        m_Registry->Replace(ecsEntity, SkyAtmosphereComponent{});
        break;
    case EntityType::VolumetricClouds:
        m_Registry->Replace(ecsEntity, VolumetricCloudComponent{});
        break;
    case EntityType::Landscape:
        m_Registry->Replace(ecsEntity, TerrainComponent{});
        break;
    case EntityType::AudioSource:
        m_Registry->Replace(ecsEntity, AudioSourceComponent{});
        break;
    default:
        break;
    }

    if (entity.ParentId != 0) {
        we::runtime::ecs::HierarchySystem hierarchy;
        hierarchy.SetParent(*m_Registry, ecsEntity, we::runtime::ecs::Entity{ entity.ParentId });
    }
}

void Scene::CreateEntity(const std::string& name, EntityType type) {
    if (type == EntityType::VolumetricClouds && HasEntityOfType(EntityType::VolumetricClouds)) {
        HE_ERROR("Only one Cloud actor is allowed per level; ignored duplicate spawn.");
        return;
    }

    const we::runtime::ecs::Entity ecsEntity = m_Registry->Create(name);

    Entity entity{};
    entity.Id = ecsEntity.id;
    entity.Name = name;
    entity.Type = type;
    ApplyDefaultEntityProperties(entity, type);
    AttachEcsComponents(entity, ecsEntity.id);
    m_Entities.push_back(entity);
    HE_INFO("Created scene entity: " + name);
}

bool Scene::HasEntityOfType(EntityType type) const {
    for (const Entity& entity : m_Entities) {
        if (entity.Type == type) {
            return true;
        }
    }
    return false;
}

bool Scene::HasEntityNamed(const std::string& name) const {
    for (const Entity& entity : m_Entities) {
        if (entity.Name == name) {
            return true;
        }
    }
    return false;
}

int Scene::FindEntityIndexById(std::uint64_t id) const {
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].Id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Scene::FindEntityIndexByName(const std::string& name) const {
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].Name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Entity* Scene::FindEntityById(std::uint64_t id) {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_Entities[static_cast<size_t>(index)] : nullptr;
}

const Entity* Scene::FindEntityById(std::uint64_t id) const {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_Entities[static_cast<size_t>(index)] : nullptr;
}

std::vector<int> Scene::FindChildIndices(std::uint64_t parentId) const {
    std::vector<int> children;
    for (size_t i = 0; i < m_Entities.size(); ++i) {
        if (m_Entities[i].ParentId == parentId) {
            children.push_back(static_cast<int>(i));
        }
    }
    return children;
}

bool Scene::IsEmpty() const {
    return m_Entities.empty();
}

void Scene::Clear() {
    DestroyEntity(0xFFFFFFFF);
}

void Scene::DestroyEntity(size_t index) {
    if (index == 0xFFFFFFFF) {
        for (const Entity& entity : m_Entities) {
            m_Registry->Destroy(we::runtime::ecs::Entity{ entity.Id });
        }
        m_Entities.clear();
        m_SelectedEntityIndex = -1;
        m_SelectedEntityId = 0;
        return;
    }

    if (index >= m_Entities.size()) return;

    const std::uint64_t destroyedId = m_Entities[index].Id;
    m_Registry->Destroy(we::runtime::ecs::Entity{ destroyedId });
    m_Entities.erase(m_Entities.begin() + static_cast<std::ptrdiff_t>(index));

    for (auto& childEntity : m_Entities) {
        if (childEntity.ParentId == destroyedId) {
            childEntity.ParentId = 0;
            if (HierarchyComponent* h = m_Registry->TryGet<HierarchyComponent>(
                    we::runtime::ecs::Entity{ childEntity.Id })) {
                h->parent = {};
            }
        }
    }

    if (m_SelectedEntityId == destroyedId) {
        m_SelectedEntityId = 0;
        m_SelectedEntityIndex = -1;
    } else if (m_SelectedEntityIndex == static_cast<int>(index)) {
        m_SelectedEntityIndex = -1;
        m_SelectedEntityId = 0;
    } else if (m_SelectedEntityIndex > static_cast<int>(index)) {
        m_SelectedEntityIndex--;
    }
}

void Scene::SetSelectedEntityIndex(int index) {
    m_SelectedEntityIndex = index;
    if (index >= 0 && index < static_cast<int>(m_Entities.size())) {
        m_SelectedEntityId = m_Entities[static_cast<std::size_t>(index)].Id;
    } else {
        m_SelectedEntityId = 0;
    }
}

void Scene::SetSelectedEntityId(std::uint64_t id) {
    m_SelectedEntityId = id;
    m_SelectedEntityIndex = FindEntityIndexById(id);
}

void Scene::SyncLegacyToEcs() {
    we::runtime::ecs::HierarchySystem hierarchy;
    for (Entity& entity : m_Entities) {
        const we::runtime::ecs::Entity ecs{ entity.Id };
        if (!m_Registry->Valid(ecs)) {
            continue;
        }

        if (NameComponent* name = m_Registry->TryGet<NameComponent>(ecs)) {
            name->value = entity.Name;
        } else {
            m_Registry->Add<NameComponent>(ecs, NameComponent{ entity.Name });
        }

        if (TransformComponent* t = m_Registry->TryGet<TransformComponent>(ecs)) {
            if (t->localPosition != entity.Position
                || t->localRotation != entity.Rotation
                || t->localScale != entity.Scale) {
                t->localPosition = entity.Position;
                t->localRotation = entity.Rotation;
                t->localScale = entity.Scale;
                t->dirty = true;
            }
        }

        if (LegacyActorComponent* legacy = m_Registry->TryGet<LegacyActorComponent>(ecs)) {
            legacy->entityType = static_cast<int>(entity.Type);
            legacy->editorOnly = entity.EditorOnly;
            legacy->mode = entity.Mode;
        }

        if (MaterialComponent* mat = m_Registry->TryGet<MaterialComponent>(ecs)) {
            mat->color = entity.Color;
        }

        HierarchyComponent* h = m_Registry->TryGet<HierarchyComponent>(ecs);
        const std::uint64_t ecsParent = h && h->parent ? h->parent.id : 0;
        if (ecsParent != entity.ParentId) {
            hierarchy.SetParent(*m_Registry, ecs,
                entity.ParentId ? we::runtime::ecs::Entity{ entity.ParentId }
                                : we::runtime::ecs::Entity{});
        }
    }
}

void Scene::SyncEcsToLegacy() {
    for (Entity& entity : m_Entities) {
        const we::runtime::ecs::Entity ecs{ entity.Id };
        if (!m_Registry->Valid(ecs)) {
            continue;
        }

        if (const NameComponent* name = m_Registry->TryGet<NameComponent>(ecs)) {
            entity.Name = name->value;
        }
        if (const TransformComponent* t = m_Registry->TryGet<TransformComponent>(ecs)) {
            entity.Position = t->localPosition;
            entity.Rotation = t->localRotation;
            entity.Scale = t->localScale;
        }
        if (const HierarchyComponent* h = m_Registry->TryGet<HierarchyComponent>(ecs)) {
            entity.ParentId = h->parent ? h->parent.id : 0;
        }
        if (const LegacyActorComponent* legacy = m_Registry->TryGet<LegacyActorComponent>(ecs)) {
            entity.EditorOnly = legacy->editorOnly;
            entity.Mode = legacy->mode;
        }
        if (const MaterialComponent* mat = m_Registry->TryGet<MaterialComponent>(ecs)) {
            entity.Color = mat->color;
        }
    }
}

void Scene::Update() {
    SyncLegacyToEcs();
    m_Systems->Update(*m_Registry, 1.0f / 60.0f);
    SyncEcsToLegacy();
}

} // namespace we::runtime::scene
