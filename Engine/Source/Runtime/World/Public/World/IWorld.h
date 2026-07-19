#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <span>
#include <string_view>

namespace we::runtime::ecs {
class Registry;
class World;
class SystemScheduler;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {

class IWorldContext;
class ILevel;
class IActorRegistry;
class ISceneGraph;
class IHierarchyService;
class ITransformHierarchy;
class ITagSystem;
class ILayerSystem;
class IQuerySystem;
class ISpatialQuery;
class ITickSystem;
class IWorldStreamer;
class IWorldPartition;
class IWorldPersistence;
class IPrefabInstancer;
class IOriginRebasing;
class IWorldSubsystemRegistry;

/// Primary world instance — owns levels, actor registry, and tick orchestration.
/// ECS storage remains owned by Scene/Registry; this interface owns identity & lifecycle policy.
class WORLD_API IWorld {
public:
    virtual ~IWorld() = default;

    [[nodiscard]] virtual WorldHandle Handle() const noexcept = 0;
    [[nodiscard]] virtual const WorldGuid& Guid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual WorldState State() const noexcept = 0;
    [[nodiscard]] virtual const WorldDescriptor& Descriptor() const noexcept = 0;

    [[nodiscard]] virtual IWorldContext& Context() noexcept = 0;
    [[nodiscard]] virtual const IWorldContext& Context() const noexcept = 0;

    [[nodiscard]] virtual IActorRegistry& Actors() noexcept = 0;
    [[nodiscard]] virtual const IActorRegistry& Actors() const noexcept = 0;
    [[nodiscard]] virtual ISceneGraph& SceneGraph() noexcept = 0;
    [[nodiscard]] virtual IHierarchyService& Hierarchy() noexcept = 0;
    [[nodiscard]] virtual ITransformHierarchy& Transforms() noexcept = 0;
    [[nodiscard]] virtual ITagSystem& Tags() noexcept = 0;
    [[nodiscard]] virtual ILayerSystem& Layers() noexcept = 0;
    [[nodiscard]] virtual IQuerySystem& Queries() noexcept = 0;
    [[nodiscard]] virtual ISpatialQuery& Spatial() noexcept = 0;
    [[nodiscard]] virtual ITickSystem& Ticks() noexcept = 0;
    [[nodiscard]] virtual IWorldStreamer& Streamer() noexcept = 0;
    [[nodiscard]] virtual IWorldPartition& Partition() noexcept = 0;
    [[nodiscard]] virtual IWorldPersistence& Persistence() noexcept = 0;
    [[nodiscard]] virtual IPrefabInstancer& Prefabs() noexcept = 0;
    [[nodiscard]] virtual IOriginRebasing& OriginRebasing() noexcept = 0;
    [[nodiscard]] virtual IWorldSubsystemRegistry& Subsystems() noexcept = 0;

    [[nodiscard]] virtual LevelHandle CreateLevel(const LevelDescriptor& desc) = 0;
    [[nodiscard]] virtual bool DestroyLevel(LevelHandle level) = 0;
    [[nodiscard]] virtual ILevel* FindLevel(LevelHandle level) noexcept = 0;
    [[nodiscard]] virtual const ILevel* FindLevel(LevelHandle level) const noexcept = 0;
    [[nodiscard]] virtual ILevel* FindLevelByGuid(const WorldGuid& guid) noexcept = 0;
    [[nodiscard]] virtual std::span<const LevelHandle> Levels() const noexcept = 0;
    [[nodiscard]] virtual LevelHandle PersistentLevel() const noexcept = 0;

    /// Active Scene used for ECS authority (persistent level by default).
    [[nodiscard]] virtual scene::Scene* ActiveScene() noexcept = 0;
    [[nodiscard]] virtual const scene::Scene* ActiveScene() const noexcept = 0;
    [[nodiscard]] virtual ecs::Registry* ActiveRegistry() noexcept = 0;
    [[nodiscard]] virtual ecs::World* ActiveEcsWorld() noexcept = 0;
    [[nodiscard]] virtual ecs::SystemScheduler* ActiveSystems() noexcept = 0;

    virtual void Tick(const WorldTickParams& params) = 0;
    virtual void BeginPlay() = 0;
    virtual void EndPlay(EndPlayReason reason) = 0;
    virtual void Suspend() = 0;
    virtual void Resume() = 0;
    virtual void Shutdown() = 0;
};

} // namespace we::runtime::world
