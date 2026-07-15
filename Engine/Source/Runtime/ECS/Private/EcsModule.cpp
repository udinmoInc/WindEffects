#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"
#include "ECS/ComponentType.h"
#include "ECS/Components/CoreComponents.h"

class EcsModule : public we::core::IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        using namespace we::runtime::ecs;
        auto& types = ComponentTypeRegistry::Get();
        types.Register<NameComponent>("Name");
        types.Register<TagComponent>("Tag");
        types.Register<UuidComponent>("Uuid");
        types.Register<TransformComponent>("Transform");
        types.Register<HierarchyComponent>("Hierarchy");
        types.Register<VisibilityComponent>("Visibility");
        types.Register<CameraComponent>("Camera");
        types.Register<DirectionalLightComponent>("DirectionalLight");
        types.Register<PointLightComponent>("PointLight");
        types.Register<SpotLightComponent>("SpotLight");
        types.Register<StaticMeshComponent>("StaticMesh");
        types.Register<MaterialComponent>("Material");
        types.Register<SkyAtmosphereComponent>("SkyAtmosphere");
        types.Register<VolumetricCloudComponent>("VolumetricCloud");
        types.Register<TerrainComponent>("Terrain");
        types.Register<WaterComponent>("Water");
        types.Register<ColliderComponent>("Collider");
        types.Register<RigidBodyComponent>("RigidBody");
        types.Register<AudioSourceComponent>("AudioSource");
        types.Register<ScriptComponent>("Script");
        types.Register<AnimationComponent>("Animation");
        types.Register<LODComponent>("LOD");
        types.Register<LegacyActorComponent>("LegacyActor");
        WE_LOG_TRACE("Plugin", "EcsModule started");
    }

    virtual void ShutdownModule() override
    {
        WE_LOG_TRACE("Plugin", "EcsModule shutdown");
    }
};

IMPLEMENT_MODULE(EcsModule, WindEffects_ECS)
