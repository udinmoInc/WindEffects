#include "Scene/Scene.h"

#include "Core/Logger.h"
#include "ECS/Registry.h"
#include "ECS/System.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/RenderExtract.h"
#include "ECS/World.h"

#include <cstring>


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
using we::runtime::ecs::VisibilityComponent;

void ApplyDefaultEntityProperties(Entity& entity, EntityType type) {
    entity.Mode = 0;

    switch (type) {
    case EntityType::Cube:
        entity.Color = we::math::Vec4(0.35f, 0.55f, 0.95f, 0.85f);
        entity.Position = we::math::Vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Sphere:
        entity.Color = we::math::Vec4(0.45f, 0.75f, 0.55f, 0.85f);
        entity.Position = we::math::Vec3(0.0f, 0.5f, 0.0f);
        entity.Scale = we::math::Vec3(1.0f);
        break;
    case EntityType::Cylinder:
        entity.Color = we::math::Vec4(0.85f, 0.55f, 0.35f, 0.85f);
        entity.Position = we::math::Vec3(0.0f, 0.5f, 0.0f);
        break;
    case EntityType::Plane:
        entity.Color = we::math::Vec4(0.5f, 0.5f, 0.5f, 1.0f);
        entity.Scale = we::math::Vec3(4.0f, 1.0f, 4.0f);
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
        entity.Color = we::math::Vec4(0.165f, 0.165f, 0.165f, 1.0f);
        entity.Position = we::math::Vec3(0.0f, 0.0f, 0.0f);
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

Entity ProjectFromEcs(const we::runtime::ecs::Registry& registry, we::runtime::ecs::Entity ecs) {
    Entity view{};
    view.Id = ecs.id;

    if (const NameComponent* name = registry.TryGet<NameComponent>(ecs)) {
        view.Name = name->value;
    }
    if (const TransformComponent* t = registry.TryGet<TransformComponent>(ecs)) {
        view.Position = t->localPosition;
        view.Rotation = t->localRotation;
        view.Scale = t->localScale;
    }
    if (const HierarchyComponent* h = registry.TryGet<HierarchyComponent>(ecs)) {
        view.ParentId = h->parent ? h->parent.id : 0;
    }
    if (const LegacyActorComponent* legacy = registry.TryGet<LegacyActorComponent>(ecs)) {
        view.Type = static_cast<EntityType>(legacy->entityType);
        view.EditorOnly = legacy->editorOnly;
        view.Mode = legacy->mode;
    }
    if (const MaterialComponent* mat = registry.TryGet<MaterialComponent>(ecs)) {
        view.Color = mat->color;
    }
    return view;
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
    Clear();
}

we::runtime::ecs::Registry& Scene::Registry() {
    return *m_Registry;
}

const we::runtime::ecs::Registry& Scene::Registry() const {
    return *m_Registry;
}

we::runtime::ecs::World& Scene::World() {
    return m_Registry->GetWorld();
}

const we::runtime::ecs::World& Scene::World() const {
    return m_Registry->GetWorld();
}

we::runtime::ecs::SystemScheduler& Scene::Systems() {
    return *m_Systems;
}

we::runtime::ecs::RenderExtractionSystem* Scene::FindExtractionSystem() const {
    for (const auto& system : m_Systems->Systems()) {
        if (!system) {
            continue;
        }
        // Prefer name match — avoids RTTI dependency (/GR- on some toolchains).
        if (std::strcmp(system->Name(), "RenderExtractionSystem") == 0) {
            return static_cast<we::runtime::ecs::RenderExtractionSystem*>(system.get());
        }
    }
    return nullptr;
}

const we::runtime::ecs::ExtractedFrameData* Scene::GetExtractedFrame() const {
    if (const we::runtime::ecs::RenderExtractionSystem* extract = FindExtractionSystem()) {
        return &extract->FrameData();
    }
    return nullptr;
}

void Scene::AttachEcsComponents(const Entity& entity, std::uint64_t ecsEntityId) {
    const we::runtime::ecs::Entity ecsEntity{ ecsEntityId };
    NameComponent nameComp{};
    const std::size_t maxChars = sizeof(nameComp.value) - 1u;
    std::size_t i = 0;
    for (; i < maxChars && i < entity.Name.size(); ++i) {
        nameComp.value[i] = entity.Name[i];
    }
    nameComp.value[i] = '\0';
    m_Registry->Replace(ecsEntity, nameComp);

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
        m_Registry->Replace(ecsEntity, StaticMeshComponent{ 1, {} });
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

    // ECS is the source of truth — create there first.
    const we::runtime::ecs::Entity ecsEntity = m_Registry->Create(name);

    Entity entity{};
    entity.Id = ecsEntity.id;
    entity.Name = name;
    entity.Type = type;
    ApplyDefaultEntityProperties(entity, type);
    AttachEcsComponents(entity, ecsEntity.id);
    m_ViewCache.push_back(entity);
    HE_INFO("Created scene entity: " + name);
}

bool Scene::HasEntityOfType(EntityType type) const {
    for (const Entity& entity : m_ViewCache) {
        if (entity.Type == type) {
            return true;
        }
    }
    return false;
}

bool Scene::HasEntityNamed(const std::string& name) const {
    for (const Entity& entity : m_ViewCache) {
        if (entity.Name == name) {
            return true;
        }
    }
    return false;
}

int Scene::FindEntityIndexById(std::uint64_t id) const {
    for (size_t i = 0; i < m_ViewCache.size(); ++i) {
        if (m_ViewCache[i].Id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Scene::FindEntityIndexByName(const std::string& name) const {
    for (size_t i = 0; i < m_ViewCache.size(); ++i) {
        if (m_ViewCache[i].Name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Entity* Scene::FindEntityById(std::uint64_t id) {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_ViewCache[static_cast<size_t>(index)] : nullptr;
}

const Entity* Scene::FindEntityById(std::uint64_t id) const {
    const int index = FindEntityIndexById(id);
    return index >= 0 ? &m_ViewCache[static_cast<size_t>(index)] : nullptr;
}

std::vector<int> Scene::FindChildIndices(std::uint64_t parentId) const {
    std::vector<int> children;
    for (size_t i = 0; i < m_ViewCache.size(); ++i) {
        if (m_ViewCache[i].ParentId == parentId) {
            children.push_back(static_cast<int>(i));
        }
    }
    return children;
}

bool Scene::IsEmpty() const {
    return m_Registry->LivingCount() == 0;
}

void Scene::Clear() {
    DestroyEntity(0xFFFFFFFF);
}

void Scene::DestroyEntity(size_t index) {
    if (index == 0xFFFFFFFF) {
        m_Registry->Clear();
        m_ViewCache.clear();
        m_SelectedEntityIndex = -1;
        m_SelectedEntityId = 0;
        return;
    }

    if (index >= m_ViewCache.size()) {
        return;
    }

    const std::uint64_t destroyedId = m_ViewCache[index].Id;
    m_Registry->Destroy(we::runtime::ecs::Entity{ destroyedId });
    m_ViewCache.erase(m_ViewCache.begin() + static_cast<std::ptrdiff_t>(index));

    for (auto& childEntity : m_ViewCache) {
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
    if (index >= 0 && index < static_cast<int>(m_ViewCache.size())) {
        m_SelectedEntityId = m_ViewCache[static_cast<std::size_t>(index)].Id;
    } else {
        m_SelectedEntityId = 0;
    }
}

void Scene::SetSelectedEntityId(std::uint64_t id) {
    m_SelectedEntityId = id;
    m_SelectedEntityIndex = FindEntityIndexById(id);
}

void Scene::PushViewEntityToEcs(const Entity& entity) {
    const we::runtime::ecs::Entity ecs{ entity.Id };
    if (!m_Registry->Valid(ecs)) {
        return;
    }

    if (NameComponent* name = m_Registry->TryGet<NameComponent>(ecs)) {
        const std::size_t maxChars = sizeof(name->value) - 1u;
        std::size_t i = 0;
        for (; i < maxChars && i < entity.Name.size(); ++i) {
            name->value[i] = entity.Name[i];
        }
        name->value[i] = '\0';
    } else {
        NameComponent nameComp{};
        const std::size_t maxChars = sizeof(nameComp.value) - 1u;
        std::size_t i = 0;
        for (; i < maxChars && i < entity.Name.size(); ++i) {
            nameComp.value[i] = entity.Name[i];
        }
        nameComp.value[i] = '\0';
        m_Registry->Add<NameComponent>(ecs, nameComp);
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

    // Keep mesh binding in sync for geometry actors (extract queries StaticMesh).
    switch (entity.Type) {
    case EntityType::Cube:
    case EntityType::Sphere:
    case EntityType::Cylinder:
    case EntityType::Plane:
    case EntityType::GroundPlane:
        if (!m_Registry->Has<StaticMeshComponent>(ecs)) {
            m_Registry->Add<StaticMeshComponent>(ecs, StaticMeshComponent{ 1, {} });
        } else if (StaticMeshComponent* mesh = m_Registry->TryGet<StaticMeshComponent>(ecs)) {
            if (mesh->meshAssetId == 0) {
                mesh->meshAssetId = 1;
            }
        }
        break;
    default:
        break;
    }

    HierarchyComponent* h = m_Registry->TryGet<HierarchyComponent>(ecs);
    const std::uint64_t ecsParent = h && h->parent ? h->parent.id : 0;
    if (ecsParent != entity.ParentId) {
        we::runtime::ecs::HierarchySystem hierarchy;
        hierarchy.SetParent(*m_Registry, ecs,
            entity.ParentId ? we::runtime::ecs::Entity{ entity.ParentId }
                            : we::runtime::ecs::Entity{});
    }
}

void Scene::SyncViewToEcs() {
    for (const Entity& entity : m_ViewCache) {
        PushViewEntityToEcs(entity);
    }
}

void Scene::RebuildViewFromEcs() {
    m_ViewCache.clear();
    m_Registry->GetWorld().Entities().ForEachLiving([&](we::runtime::ecs::Entity ecs) {
        // Skip entities without LegacyActor (pure runtime/system entities).
        if (!m_Registry->Has<LegacyActorComponent>(ecs)) {
            return;
        }
        m_ViewCache.push_back(ProjectFromEcs(*m_Registry, ecs));
    });

    if (m_SelectedEntityId != 0) {
        m_SelectedEntityIndex = FindEntityIndexById(m_SelectedEntityId);
        if (m_SelectedEntityIndex < 0) {
            m_SelectedEntityId = 0;
        }
    }
}

void Scene::SyncLegacyToEcs() {
    SyncViewToEcs();
}

void Scene::SyncEcsToLegacy() {
    RebuildViewFromEcs();
}

void Scene::Update() {
    Update(1.0f / 60.0f);
}

void Scene::Update(float deltaSeconds) {
    // 1) Editor view mutations → ECS
    SyncViewToEcs();
    // 2) Systems (transform, extract, …) on ECS World
    m_Systems->Update(*m_Registry, deltaSeconds);
    // 3) Rebuild editor projection from ECS authority
    RebuildViewFromEcs();
}

} // namespace we::runtime::scene
