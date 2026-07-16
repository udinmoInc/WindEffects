#include "ECS/ComponentType.h"
#include "ECS/ComponentOps.h"
#include "ECS/Components/CoreComponents.h"

namespace we::runtime::ecs {

ComponentTypeRegistry::ComponentTypeRegistry() {
    m_Types.resize(static_cast<std::size_t>(CoreComponentId::Count));
}

ComponentTypeRegistry& ComponentTypeRegistry::Get() {
    static ComponentTypeRegistry instance;
    return instance;
}

void ComponentTypeRegistry::EnsureCoreTypesRegistered() {
    if (m_CoreRegistered) {
        return;
    }
    m_CoreRegistered = true;

    Register<NameComponent>("Name");
    Register<TagComponent>("Tag");
    Register<UuidComponent>("Uuid");
    Register<TransformComponent>("Transform");
    Register<HierarchyComponent>("Hierarchy");
    Register<VisibilityComponent>("Visibility");
    Register<CameraComponent>("Camera");
    Register<DirectionalLightComponent>("DirectionalLight");
    Register<PointLightComponent>("PointLight");
    Register<SpotLightComponent>("SpotLight");
    Register<StaticMeshComponent>("StaticMesh");
    Register<MaterialComponent>("Material");
    Register<SkyAtmosphereComponent>("SkyAtmosphere");
    Register<VolumetricCloudComponent>("VolumetricCloud");
    Register<TerrainComponent>("Terrain");
    Register<WaterComponent>("Water");
    Register<ColliderComponent>("Collider");
    Register<RigidBodyComponent>("RigidBody");
    Register<AudioSourceComponent>("AudioSource");
    Register<ScriptComponent>("Script");
    Register<AnimationComponent>("Animation");
    Register<LODComponent>("LOD");
    Register<LegacyActorComponent>("LegacyActor");

    RegisterComponentOps(Id<TransformComponent>(), MakeOpsFor<TransformComponent>());
    RegisterComponentOps(Id<HierarchyComponent>(), MakeOpsFor<HierarchyComponent>());
    RegisterComponentOps(Id<VisibilityComponent>(), MakeOpsFor<VisibilityComponent>());
    RegisterComponentOps(Id<NameComponent>(), MakeOpsFor<NameComponent>());
    RegisterComponentOps(Id<TagComponent>(), MakeOpsFor<TagComponent>());
    RegisterComponentOps(Id<StaticMeshComponent>(), MakeOpsFor<StaticMeshComponent>());
    RegisterComponentOps(Id<MaterialComponent>(), MakeOpsFor<MaterialComponent>());
    RegisterComponentOps(Id<UuidComponent>(), MakeOpsFor<UuidComponent>());
    RegisterComponentOps(Id<AudioSourceComponent>(), MakeOpsFor<AudioSourceComponent>());
    RegisterComponentOps(Id<ScriptComponent>(), MakeOpsFor<ScriptComponent>());
    RegisterComponentOps(Id<AnimationComponent>(), MakeOpsFor<AnimationComponent>());
}

ComponentTypeId ComponentTypeRegistry::RegisterType(
    ComponentTypeId id,
    const char* name,
    std::size_t size,
    std::size_t alignment,
    ComponentCategory category,
    bool enableable)
{
    if (id >= kMaxComponentTypes) {
        return kInvalidComponentType;
    }
    if (id >= m_Types.size()) {
        m_Types.resize(static_cast<std::size_t>(id) + 1u);
    }
    ComponentTypeInfo& info = m_Types[id];
    if (info.id == kInvalidComponentType) {
        info.id = id;
        info.name = name ? name : "Component";
        info.size = size;
        info.alignment = alignment;
        info.category = category;
        info.enableable = enableable;
        info.mask.Set(id);
    }
    return id;
}

ComponentTypeId ComponentTypeRegistry::RegisterDynamic(
    const char* name,
    std::size_t size,
    std::size_t alignment,
    ComponentCategory category,
    bool enableable)
{
    if (m_NextDynamicId >= kMaxComponentTypes) {
        return kInvalidComponentType;
    }
    return RegisterType(m_NextDynamicId++, name, size, alignment, category, enableable);
}

const ComponentTypeInfo* ComponentTypeRegistry::Find(ComponentTypeId id) const {
    if (id >= m_Types.size()) {
        return nullptr;
    }
    if (m_Types[id].id == kInvalidComponentType) {
        return nullptr;
    }
    return &m_Types[id];
}

} // namespace we::runtime::ecs
