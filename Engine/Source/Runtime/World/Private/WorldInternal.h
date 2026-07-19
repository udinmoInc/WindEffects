#pragma once

#include "World/IActor.h"
#include "World/IHierarchy.h"
#include "World/ILevel.h"
#include "World/IPrefabOrigin.h"
#include "World/IQuery.h"
#include "World/IStreaming.h"
#include "World/ITagLayer.h"
#include "World/ITickSystem.h"
#include "World/ITransformHierarchy.h"
#include "World/IWorld.h"
#include "World/IWorldContext.h"
#include "World/IWorldManager.h"
#include "World/IWorldPersistence.h"
#include "World/IWorldRegistry.h"
#include "World/IWorldRuntime.h"
#include "World/IWorldSubsystem.h"
#include "World/Integration/IWorldHooks.h"
#include "World/WorldDiagnostics.h"
#include "World/WorldTypes.h"

#include "Scene/Scene.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/Registry.h"
#include "ECS/System.h"
#include "ECS/World.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::world::detail {

inline void CopyName(char (&dst)[64], std::string_view src) {
    const std::size_t n = src.size() < 63 ? src.size() : 63;
    if (n > 0) {
        std::memcpy(dst, src.data(), n);
    }
    dst[n] = '\0';
}

inline std::string_view NameView(const char (&src)[64]) {
    return std::string_view(src);
}

template <typename HandleT>
struct GenerationalSlot {
    std::uint32_t generation = 0;
    bool occupied = false;
};

class WorldImpl;

class ActorImpl final : public IActor {
public:
    ActorHandle handle{};
    WorldGuid guid{};
    char name[64]{};
    ActorLifecycle lifecycle = ActorLifecycle::Constructed;
    WorldHandle world{};
    LevelHandle level{};
    LayerId layer{};
    TagId tags{};
    std::uint64_t entityId = 0;
    ActorHandle parent{};
    std::vector<ActorHandle> children;
    Vec3f localPosition{};
    Vec3f localRotation{};
    Vec3f localScale{1.f, 1.f, 1.f};
    bool pendingBeginPlay = false;
    bool pendingEndPlay = false;
    EndPlayReason endPlayReason = EndPlayReason::Destroyed;

    [[nodiscard]] ActorHandle Handle() const noexcept override { return handle; }
    [[nodiscard]] const WorldGuid& Guid() const noexcept override { return guid; }
    [[nodiscard]] std::string_view Name() const noexcept override { return NameView(name); }
    [[nodiscard]] ActorLifecycle Lifecycle() const noexcept override { return lifecycle; }
    [[nodiscard]] WorldHandle OwningWorld() const noexcept override { return world; }
    [[nodiscard]] LevelHandle OwningLevel() const noexcept override { return level; }
    [[nodiscard]] LayerId Layer() const noexcept override { return layer; }
    [[nodiscard]] TagId Tags() const noexcept override { return tags; }
    [[nodiscard]] std::uint64_t EntityId() const noexcept override { return entityId; }
    [[nodiscard]] bool IsValid() const noexcept override {
        return handle.IsValid() && lifecycle != ActorLifecycle::Destroyed;
    }
    [[nodiscard]] bool HasBegunPlay() const noexcept override {
        return lifecycle == ActorLifecycle::Playing;
    }
    void SetName(std::string_view n) override { CopyName(name, n); }
    void SetLayer(LayerId l) override { layer = l; }
    void SetTags(TagId t) override { tags = t; }
    void BeginPlay() override;
    void EndPlay(EndPlayReason reason) override;
    void Tick(float) override {}
};

class ActorRegistryImpl final : public IActorRegistry {
public:
    explicit ActorRegistryImpl(WorldImpl& world);

    ActorHandle Spawn(const ActorSpawnParams& params, LevelHandle level) override;
    bool Destroy(ActorHandle actor, EndPlayReason reason) override;
    IActor* TryGet(ActorHandle handle) noexcept override;
    const IActor* TryGet(ActorHandle handle) const noexcept override;
    IActor* TryGetByGuid(const WorldGuid& guid) noexcept override;
    IActor* TryGetByEntityId(std::uint64_t entityId) noexcept override;
    IActor* TryGetByName(std::string_view name) noexcept override;
    std::span<const ActorHandle> All() const noexcept override;
    std::size_t Count() const noexcept override;
    void FlushPendingBeginPlay() override;
    void FlushPendingEndPlay() override;

    WorldImpl& m_World;
    std::vector<std::unique_ptr<ActorImpl>> m_Slots;
    std::vector<ActorHandle> m_All;
    std::unordered_map<WorldGuid, ActorHandle, WorldGuidHash> m_ByGuid;
    std::unordered_map<std::uint64_t, ActorHandle> m_ByEntity;
    std::uint32_t m_NextGeneration = 1;
    mutable std::shared_mutex m_Mutex;
};

class HierarchyServiceImpl final : public IHierarchyService {
public:
    explicit HierarchyServiceImpl(WorldImpl& world);
    bool SetParent(ActorHandle child, ActorHandle parent) override;
    bool Detach(ActorHandle child) override;
    ActorHandle GetParent(ActorHandle actor) const noexcept override;
    std::span<const ActorHandle> GetChildren(ActorHandle actor) const override;
    bool IsAncestorOf(ActorHandle ancestor, ActorHandle descendant) const noexcept override;
    std::uint16_t Depth(ActorHandle actor) const noexcept override;
    void TraverseDepthFirst(ActorHandle root, const VisitFn& visit) const override;
    void TraverseBreadthFirst(ActorHandle root, const VisitFn& visit) const override;

    WorldImpl& m_World;
    mutable std::vector<ActorHandle> m_ChildScratch;
};

class SceneGraphImpl final : public ISceneGraph {
public:
    explicit SceneGraphImpl(WorldImpl& world);
    ActorHandle Root() const noexcept override;
    IHierarchyService& Hierarchy() noexcept override;
    std::span<const ActorHandle> Roots() const override;
    void Rebuild() override;

    WorldImpl& m_World;
    ActorHandle m_Root{};
    mutable std::vector<ActorHandle> m_Roots;
};

class TransformHierarchyImpl final : public ITransformHierarchy {
public:
    explicit TransformHierarchyImpl(WorldImpl& world);
    void SetLocalPosition(ActorHandle actor, const Vec3f& position) override;
    void SetLocalRotationEuler(ActorHandle actor, const Vec3f& eulerDegrees) override;
    void SetLocalScale(ActorHandle actor, const Vec3f& scale) override;
    Vec3f GetLocalPosition(ActorHandle actor) const override;
    Vec3f GetLocalRotationEuler(ActorHandle actor) const override;
    Vec3f GetLocalScale(ActorHandle actor) const override;
    Vec3f GetWorldPosition(ActorHandle actor) const override;
    void MarkDirty(ActorHandle actor) override;
    void UpdateWorldTransforms() override;
    void SyncToEcs() override;
    void SyncFromEcs() override;

    WorldImpl& m_World;
};

class TagSystemImpl final : public ITagSystem {
public:
    explicit TagSystemImpl(WorldImpl& world);
    TagId RegisterTag(std::string_view name) override;
    TagId FindTag(std::string_view name) const noexcept override;
    std::string_view TagName(TagId id) const noexcept override;
    void AddTag(ActorHandle actor, TagId tag) override;
    void RemoveTag(ActorHandle actor, TagId tag) override;
    bool HasTag(ActorHandle actor, TagId tag) const noexcept override;
    TagId GetTags(ActorHandle actor) const noexcept override;
    std::span<const ActorHandle> QueryTag(TagId tag) const override;

    WorldImpl& m_World;
    std::unordered_map<std::string, TagId> m_NameToId;
    std::unordered_map<std::uint64_t, std::string> m_IdToName;
    std::uint64_t m_NextBit = 1;
    mutable std::vector<ActorHandle> m_QueryScratch;
};

class LayerSystemImpl final : public ILayerSystem {
public:
    explicit LayerSystemImpl(WorldImpl& world);
    LayerId RegisterLayer(std::string_view name) override;
    LayerId FindLayer(std::string_view name) const noexcept override;
    std::string_view LayerName(LayerId id) const noexcept override;
    void SetLayer(ActorHandle actor, LayerId layer) override;
    LayerId GetLayer(ActorHandle actor) const noexcept override;
    std::span<const ActorHandle> QueryLayer(LayerId layer) const override;
    void SetLayerVisible(LayerId layer, bool visible) override;
    bool IsLayerVisible(LayerId layer) const noexcept override;

    WorldImpl& m_World;
    std::unordered_map<std::string, LayerId> m_NameToId;
    std::unordered_map<std::uint32_t, std::string> m_IdToName;
    std::unordered_map<std::uint32_t, bool> m_Visibility;
    std::uint32_t m_NextId = 1;
    mutable std::vector<ActorHandle> m_QueryScratch;
};

class QuerySystemImpl final : public IQuerySystem {
public:
    explicit QuerySystemImpl(WorldImpl& world);
    std::vector<ActorHandle> Query(const ActorQueryFilter& filter) const override;
    std::vector<ActorHandle> QueryByName(std::string_view name) const override;
    std::vector<ActorHandle> QueryWhere(const ActorPredicate& predicate) const override;
    ActorHandle FindFirst(const ActorQueryFilter& filter) const override;

    WorldImpl& m_World;
};

class SpatialQueryImpl final : public ISpatialQuery {
public:
    explicit SpatialQueryImpl(WorldImpl& world);
    void Rebuild() override;
    std::vector<ActorHandle> Query(const SpatialQueryParams& params) const override;
    std::vector<ActorHandle> OverlapSphere(const Sphere3f& sphere, const ActorQueryFilter& filter) const override;
    std::vector<ActorHandle> OverlapBox(const Aabb3f& box, const ActorQueryFilter& filter) const override;

    WorldImpl& m_World;
    struct Entry {
        ActorHandle actor{};
        Vec3f position{};
    };
    mutable std::vector<Entry> m_Entries;
};

class TickSystemImpl final : public ITickSystem {
public:
    explicit TickSystemImpl(WorldImpl& world);
    std::uint64_t Register(const TickRegistration& reg, TickFunction fn) override;
    bool Unregister(std::uint64_t tickId) override;
    void SetFixedDeltaSeconds(float seconds) override;
    float FixedDeltaSeconds() const noexcept override;
    double Accumulator() const noexcept override;
    void Tick(IWorldContext& context, const WorldTickParams& params) override;
    void FlushBeginPlay(IWorldContext& context) override;
    void FlushEndPlay(IWorldContext& context) override;

    struct Entry {
        std::uint64_t id = 0;
        TickRegistration reg{};
        TickFunction fn;
    };

    WorldImpl& m_World;
    std::vector<Entry> m_Entries;
    std::uint64_t m_NextId = 1;
    float m_FixedDt = 1.f / 60.f;
    double m_Accumulator = 0.0;
};

class WorldSubsystemRegistryImpl final : public IWorldSubsystemRegistry {
public:
    void Register(std::unique_ptr<IWorldSubsystem> subsystem) override;
    bool Unregister(std::string_view name) override;
    IWorldSubsystem* Find(std::string_view name) noexcept override;
    std::span<IWorldSubsystem* const> All() const override;
    void InitializeAll(IWorld& world) override;
    void DeinitializeAll() override;
    void BeginPlayAll(IWorldContext& context) override;
    void EndPlayAll(IWorldContext& context, EndPlayReason reason) override;

    std::vector<std::unique_ptr<IWorldSubsystem>> m_Owned;
    std::vector<IWorldSubsystem*> m_Ptrs;
};

class LevelImpl final : public ILevel {
public:
    LevelHandle handle{};
    WorldGuid guid{};
    char name[64]{};
    LevelState state = LevelState::Unloaded;
    LevelDescriptor descriptor{};
    WorldHandle owningWorld{};
    bool persistent = false;
    bool visible = true;
    Aabb3f bounds{};
    std::shared_ptr<scene::Scene> scene;
    std::vector<ActorHandle> actors;
    WorldImpl* world = nullptr;

    [[nodiscard]] LevelHandle Handle() const noexcept override { return handle; }
    [[nodiscard]] const WorldGuid& Guid() const noexcept override { return guid; }
    [[nodiscard]] std::string_view Name() const noexcept override { return NameView(name); }
    [[nodiscard]] LevelState State() const noexcept override { return state; }
    [[nodiscard]] const LevelDescriptor& Descriptor() const noexcept override { return descriptor; }
    [[nodiscard]] bool IsPersistent() const noexcept override { return persistent; }
    [[nodiscard]] WorldHandle OwningWorld() const noexcept override { return owningWorld; }
    [[nodiscard]] scene::Scene& Scene() noexcept override { return *scene; }
    [[nodiscard]] const scene::Scene& Scene() const noexcept override { return *scene; }
    [[nodiscard]] ecs::Registry& Registry() noexcept override { return scene->Registry(); }
    [[nodiscard]] ecs::World& EcsWorld() noexcept override { return scene->World(); }
    [[nodiscard]] Aabb3f Bounds() const noexcept override { return bounds; }
    void SetBounds(const Aabb3f& b) override { bounds = b; }
    [[nodiscard]] std::span<const ActorHandle> Actors() const noexcept override { return actors; }
    void SetVisible(bool v) override;
    [[nodiscard]] bool IsVisible() const noexcept override { return visible; }
    bool Load() override;
    bool Unload() override;
    void Tick(float deltaSeconds) override;
};

class WorldContextImpl final : public IWorldContext {
public:
    explicit WorldContextImpl(WorldImpl& world);

    IWorld& World() noexcept override;
    const IWorld& World() const noexcept override;
    IWorldManager* Manager() noexcept override;
    float DeltaSeconds() const noexcept override { return m_Delta; }
    float FixedDeltaSeconds() const noexcept override { return m_FixedDelta; }
    double WorldTimeSeconds() const noexcept override { return m_WorldTime; }
    std::uint64_t FrameIndex() const noexcept override { return m_Frame; }
    WorldNetMode NetMode() const noexcept override;
    bool IsPlaying() const noexcept override { return m_Playing; }
    bool IsEditor() const noexcept override { return m_Editor; }
    bool IsDedicatedServer() const noexcept override {
        return NetMode() == WorldNetMode::DedicatedServer;
    }
    void RegisterService(std::type_index type, std::shared_ptr<void> service) override;
    void UnregisterService(std::type_index type) override;
    std::shared_ptr<void> TryGetService(std::type_index type) const override;
    void SetTickParams(const WorldTickParams& params) override;

    WorldImpl& m_World;
    float m_Delta = 0.f;
    float m_FixedDelta = 1.f / 60.f;
    double m_WorldTime = 0.0;
    std::uint64_t m_Frame = 0;
    bool m_Playing = false;
    bool m_Editor = false;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

class WorldStreamerImpl final : public IWorldStreamer {
public:
    explicit WorldStreamerImpl(WorldImpl& world);
    std::future<bool> LoadLevelAsync(const StreamRequest& request, StreamProgressCallback onProgress) override;
    std::future<bool> UnloadLevelAsync(LevelHandle level, StreamProgressCallback onProgress) override;
    bool LoadLevel(const StreamRequest& request) override;
    bool UnloadLevel(LevelHandle level) override;
    std::span<const LevelHandle> StreamingLevels() const override;
    void Tick(float deltaSeconds) override;
    void CancelAll() override;

    WorldImpl& m_World;
    std::vector<LevelHandle> m_Streaming;
};

class WorldPartitionImpl final : public IWorldPartition {
public:
    explicit WorldPartitionImpl(WorldImpl& world);
    void Configure(const PartitionSettings& settings) override;
    const PartitionSettings& Settings() const noexcept override { return m_Settings; }
    PartitionCellId WorldToCell(const Vec3f& worldPosition) const noexcept override;
    void UpdateStreamingSource(const Vec3f& worldPosition) override;
    std::vector<PartitionCellId> LoadedCells() const override;
    void Tick(float deltaSeconds) override;

    WorldImpl& m_World;
    PartitionSettings m_Settings{};
    Vec3f m_Source{};
    std::vector<PartitionCellId> m_Loaded;
};

class WorldPersistenceImpl final : public IWorldPersistence {
public:
    explicit WorldPersistenceImpl(WorldImpl& world);
    void BindSerializer(serialization::ISerializer* serializer) override;
    void BindTypeRegistry(reflection::ITypeRegistry* registry) override;
    std::vector<std::uint8_t> SaveWorld(const WorldSaveOptions& options) override;
    bool LoadWorld(std::span<const std::uint8_t> bytes, const WorldLoadOptions& options) override;
    bool SaveWorldToFile(std::string_view path, const WorldSaveOptions& options) override;
    bool LoadWorldFromFile(std::string_view path, const WorldLoadOptions& options) override;
    std::future<std::vector<std::uint8_t>> SaveWorldAsync(
        WorldSaveOptions options, PersistenceProgressCallback onProgress) override;
    std::future<bool> LoadWorldAsync(
        std::vector<std::uint8_t> bytes,
        WorldLoadOptions options,
        PersistenceProgressCallback onProgress) override;

    WorldImpl& m_World;
    serialization::ISerializer* m_Serializer = nullptr;
    reflection::ITypeRegistry* m_Registry = nullptr;
};

class PrefabInstancerImpl final : public IPrefabInstancer {
public:
    explicit PrefabInstancerImpl(WorldImpl& world);
    ActorHandle Instantiate(const PrefabInstanceParams& params) override;
    bool RegisterPrefabSource(const WorldGuid& prefabGuid, std::string_view assetPath) override;
    bool UnregisterPrefabSource(const WorldGuid& prefabGuid) override;

    WorldImpl& m_World;
    std::unordered_map<WorldGuid, std::string, WorldGuidHash> m_Sources;
};

class OriginRebasingImpl final : public IOriginRebasing {
public:
    explicit OriginRebasingImpl(WorldImpl& world);
    void SetEnabled(bool enabled) override;
    bool IsEnabled() const noexcept override;
    Vec3f CurrentOrigin() const noexcept override;
    void RequestRebase(const Vec3f& newOrigin) override;
    void ApplyPendingRebase() override;
    void SetRebaseCallback(RebaseCallback callback, void* userData) override;

    WorldImpl& m_World;
    bool m_Enabled = false;
    Vec3f m_Origin{};
    Vec3f m_Pending{};
    bool m_HasPending = false;
    RebaseCallback m_Callback = nullptr;
    void* m_UserData = nullptr;
};

class WorldImpl final : public IWorld {
public:
    WorldImpl(WorldHandle handle, WorldCreateInfo info, IWorldManager* manager, WorldRuntimeDependencies runtimeDeps);
    ~WorldImpl() override;

    WorldHandle Handle() const noexcept override { return m_Handle; }
    const WorldGuid& Guid() const noexcept override { return m_Descriptor.guid; }
    std::string_view Name() const noexcept override { return NameView(m_Descriptor.name); }
    WorldState State() const noexcept override { return m_State; }
    const WorldDescriptor& Descriptor() const noexcept override { return m_Descriptor; }

    IWorldContext& Context() noexcept override { return m_Context; }
    const IWorldContext& Context() const noexcept override { return m_Context; }
    IActorRegistry& Actors() noexcept override { return m_Actors; }
    const IActorRegistry& Actors() const noexcept override { return m_Actors; }
    ISceneGraph& SceneGraph() noexcept override { return m_SceneGraph; }
    IHierarchyService& Hierarchy() noexcept override { return m_Hierarchy; }
    ITransformHierarchy& Transforms() noexcept override { return m_Transforms; }
    ITagSystem& Tags() noexcept override { return m_Tags; }
    ILayerSystem& Layers() noexcept override { return m_Layers; }
    IQuerySystem& Queries() noexcept override { return m_Queries; }
    ISpatialQuery& Spatial() noexcept override { return m_Spatial; }
    ITickSystem& Ticks() noexcept override { return m_Ticks; }
    IWorldStreamer& Streamer() noexcept override { return m_Streamer; }
    IWorldPartition& Partition() noexcept override { return m_Partition; }
    IWorldPersistence& Persistence() noexcept override { return m_Persistence; }
    IPrefabInstancer& Prefabs() noexcept override { return m_Prefabs; }
    IOriginRebasing& OriginRebasing() noexcept override { return m_Origin; }
    IWorldSubsystemRegistry& Subsystems() noexcept override { return m_Subsystems; }

    LevelHandle CreateLevel(const LevelDescriptor& desc) override;
    bool DestroyLevel(LevelHandle level) override;
    ILevel* FindLevel(LevelHandle level) noexcept override;
    const ILevel* FindLevel(LevelHandle level) const noexcept override;
    ILevel* FindLevelByGuid(const WorldGuid& guid) noexcept override;
    std::span<const LevelHandle> Levels() const noexcept override;
    LevelHandle PersistentLevel() const noexcept override { return m_PersistentLevel; }

    scene::Scene* ActiveScene() noexcept override;
    const scene::Scene* ActiveScene() const noexcept override;
    ecs::Registry* ActiveRegistry() noexcept override;
    ecs::World* ActiveEcsWorld() noexcept override;
    ecs::SystemScheduler* ActiveSystems() noexcept override;

    void Tick(const WorldTickParams& params) override;
    void BeginPlay() override;
    void EndPlay(EndPlayReason reason) override;
    void Suspend() override;
    void Resume() override;
    void Shutdown() override;

    LevelImpl* GetLevel(LevelHandle level) noexcept;
    const LevelImpl* GetLevel(LevelHandle level) const noexcept;
    ActorImpl* GetActor(ActorHandle handle) noexcept;
    const ActorImpl* GetActor(ActorHandle handle) const noexcept;

    WorldHandle m_Handle{};
    WorldDescriptor m_Descriptor{};
    WorldState m_State = WorldState::Uninitialized;
    IWorldManager* m_Manager = nullptr;
    WorldRuntimeDependencies m_RuntimeDeps{};

    WorldContextImpl m_Context;
    ActorRegistryImpl m_Actors;
    HierarchyServiceImpl m_Hierarchy;
    SceneGraphImpl m_SceneGraph;
    TransformHierarchyImpl m_Transforms;
    TagSystemImpl m_Tags;
    LayerSystemImpl m_Layers;
    QuerySystemImpl m_Queries;
    SpatialQueryImpl m_Spatial;
    TickSystemImpl m_Ticks;
    WorldStreamerImpl m_Streamer;
    WorldPartitionImpl m_Partition;
    WorldPersistenceImpl m_Persistence;
    PrefabInstancerImpl m_Prefabs;
    OriginRebasingImpl m_Origin;
    WorldSubsystemRegistryImpl m_Subsystems;

    std::vector<std::unique_ptr<LevelImpl>> m_LevelSlots;
    std::vector<LevelHandle> m_LevelHandles;
    LevelHandle m_PersistentLevel{};
    LevelHandle m_ActiveLevel{};
};

class WorldRegistryImpl final : public IWorldRegistry {
public:
    bool Contains(WorldHandle handle) const noexcept override;
    bool ContainsGuid(const WorldGuid& guid) const noexcept override;
    IWorld* TryGet(WorldHandle handle) noexcept override;
    const IWorld* TryGet(WorldHandle handle) const noexcept override;
    IWorld* TryGetByGuid(const WorldGuid& guid) noexcept override;
    IWorld* TryGetByName(std::string_view name) noexcept override;
    std::span<const WorldHandle> All() const noexcept override;
    std::size_t Count() const noexcept override;

    void Add(WorldHandle handle, WorldImpl* world);
    void Remove(WorldHandle handle);

    std::vector<WorldImpl*> m_Slots;
    std::vector<std::uint32_t> m_Generations;
    std::vector<WorldHandle> m_All;
    std::unordered_map<WorldGuid, WorldHandle, WorldGuidHash> m_ByGuid;
    mutable std::shared_mutex m_Mutex;
};

class WorldManagerImpl final : public IWorldManager {
public:
    explicit WorldManagerImpl(WorldRuntimeDependencies deps);
    ~WorldManagerImpl() override;

    IWorldRegistry& Registry() noexcept override { return m_Registry; }
    const IWorldRegistry& Registry() const noexcept override { return m_Registry; }
    WorldHandle CreateWorld(const WorldCreateInfo& info) override;
    bool DestroyWorld(WorldHandle handle) override;
    IWorld* FindWorld(WorldHandle handle) noexcept override;
    const IWorld* FindWorld(WorldHandle handle) const noexcept override;
    IWorld* FindWorldByGuid(const WorldGuid& guid) noexcept override;
    IWorld* FindWorldByName(std::string_view name) noexcept override;
    std::span<const WorldHandle> Worlds() const noexcept override;
    WorldHandle ActiveWorld() const noexcept override { return m_Active; }
    void SetActiveWorld(WorldHandle handle) override;
    void TickAll(const WorldTickParams& params) override;
    void TickWorld(WorldHandle handle, const WorldTickParams& params) override;
    void Shutdown() override;

    WorldRuntimeDependencies m_Deps{};
    WorldRegistryImpl m_Registry;
    std::vector<std::unique_ptr<WorldImpl>> m_Worlds;
    WorldHandle m_Active{};
    std::uint32_t m_NextIndex = 0;
    mutable std::mutex m_Mutex;
};

class WorldRuntimeImpl final : public IWorldRuntime {
public:
    explicit WorldRuntimeImpl(WorldRuntimeDependencies deps);
    ~WorldRuntimeImpl() override;

    IWorldManager& Manager() noexcept override { return *m_Manager; }
    const IWorldManager& Manager() const noexcept override { return *m_Manager; }
    void Tick(const WorldTickParams& params) override;
    void Shutdown() override;

    WorldRuntimeDependencies m_Deps{};
    std::unique_ptr<WorldManagerImpl> m_Manager;
};

} // namespace we::runtime::world::detail
