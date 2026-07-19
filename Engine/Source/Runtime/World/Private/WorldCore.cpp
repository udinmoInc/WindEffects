#include "WorldInternal.h"

#include <algorithm>

namespace we::runtime::world::detail {

void LevelImpl::SetVisible(bool v) {
    visible = v;
    if (v && state == LevelState::Loaded) {
        state = LevelState::Visible;
    } else if (!v && state == LevelState::Visible) {
        state = LevelState::Loaded;
    }
}

bool LevelImpl::Load() {
    if (state == LevelState::Loaded || state == LevelState::Visible) {
        return true;
    }
    state = LevelState::Loading;
    if (!scene) {
        scene = std::make_shared<scene::Scene>();
    }
    state = visible ? LevelState::Visible : LevelState::Loaded;
    WorldDiagnostics::Get().OnStreamLoad();
    return true;
}

bool LevelImpl::Unload() {
    if (state == LevelState::Unloaded) {
        return true;
    }
    state = LevelState::Unloading;
    if (!persistent && scene) {
        scene->Clear();
    }
    state = LevelState::Unloaded;
    WorldDiagnostics::Get().OnStreamUnload();
    return true;
}

void LevelImpl::Tick(float deltaSeconds) {
    if (!scene || (state != LevelState::Loaded && state != LevelState::Visible)) {
        return;
    }
    scene->Update(deltaSeconds);
}

WorldContextImpl::WorldContextImpl(WorldImpl& world) : m_World(world) {}

IWorld& WorldContextImpl::World() noexcept { return m_World; }
const IWorld& WorldContextImpl::World() const noexcept { return m_World; }
IWorldManager* WorldContextImpl::Manager() noexcept { return m_World.m_Manager; }

WorldNetMode WorldContextImpl::NetMode() const noexcept {
    return m_World.m_Descriptor.netMode;
}

void WorldContextImpl::RegisterService(std::type_index type, std::shared_ptr<void> service) {
    m_Services[type] = std::move(service);
}

void WorldContextImpl::UnregisterService(std::type_index type) {
    m_Services.erase(type);
}

std::shared_ptr<void> WorldContextImpl::TryGetService(std::type_index type) const {
    const auto it = m_Services.find(type);
    return it == m_Services.end() ? nullptr : it->second;
}

void WorldContextImpl::SetTickParams(const WorldTickParams& params) {
    m_Delta = params.deltaSeconds;
    m_FixedDelta = params.fixedDeltaSeconds;
    m_WorldTime = params.worldTimeSeconds;
    m_Frame = params.frameIndex;
}

WorldImpl::WorldImpl(
    WorldHandle handle,
    WorldCreateInfo info,
    IWorldManager* manager,
    WorldRuntimeDependencies runtimeDeps)
    : m_Handle(handle)
    , m_Descriptor(info.descriptor)
    , m_Manager(manager)
    , m_RuntimeDeps(std::move(runtimeDeps))
    , m_Context(*this)
    , m_Actors(*this)
    , m_Hierarchy(*this)
    , m_SceneGraph(*this)
    , m_Transforms(*this)
    , m_Tags(*this)
    , m_Layers(*this)
    , m_Queries(*this)
    , m_Spatial(*this)
    , m_Ticks(*this)
    , m_Streamer(*this)
    , m_Partition(*this)
    , m_Persistence(*this)
    , m_Prefabs(*this)
    , m_Origin(*this)
{
    m_Context.m_Editor = info.isEditor;
    if (m_Descriptor.guid.IsNil()) {
        m_Descriptor.guid = WorldGuid::Generate();
    }
    if (m_Descriptor.name[0] == '\0') {
        CopyName(m_Descriptor.name, "World");
    }

    m_Persistence.BindSerializer(m_RuntimeDeps.serializer);
    m_Persistence.BindTypeRegistry(m_RuntimeDeps.typeRegistry);

    if (info.createPersistentLevel) {
        LevelDescriptor levelDesc{};
        levelDesc.guid = WorldGuid::Generate();
        CopyName(levelDesc.name, "PersistentLevel");
        levelDesc.worldGuid = m_Descriptor.guid;
        levelDesc.persistent = true;
        levelDesc.streamable = false;
        m_PersistentLevel = CreateLevel(levelDesc);
        m_ActiveLevel = m_PersistentLevel;

        if (LevelImpl* level = GetLevel(m_PersistentLevel)) {
            if (info.existingScene) {
                level->scene = info.existingScene;
            } else if (!level->scene) {
                level->scene = std::make_shared<scene::Scene>();
            }
            level->persistent = true;
            level->state = LevelState::Visible;
            level->Load();
        }
    }

    if (info.registerDefaultSubsystems) {
        for (auto& subsystem : WorldSubsystemFactoryRegistry::Get().CreateAll()) {
            m_Subsystems.Register(std::move(subsystem));
        }
    }
    m_Subsystems.InitializeAll(*this);

    m_State = WorldState::Ready;
    WorldDiagnostics::Get().OnWorldCreated();

    if (m_RuntimeDeps.physicsHook) m_RuntimeDeps.physicsHook->OnWorldCreated(*this);
    if (m_RuntimeDeps.audioHook) m_RuntimeDeps.audioHook->OnWorldCreated(*this);
    if (m_RuntimeDeps.networkHook) m_RuntimeDeps.networkHook->OnWorldCreated(*this);
    if (m_RuntimeDeps.editorHook) m_RuntimeDeps.editorHook->OnWorldCreated(*this);
    if (m_RuntimeDeps.renderHook) m_RuntimeDeps.renderHook->OnWorldCreated(*this);
}

WorldImpl::~WorldImpl() {
    Shutdown();
}

LevelHandle WorldImpl::CreateLevel(const LevelDescriptor& desc) {
    std::uint32_t index = 0;
    for (; index < m_LevelSlots.size(); ++index) {
        if (!m_LevelSlots[index]) {
            break;
        }
    }
    if (index == m_LevelSlots.size()) {
        m_LevelSlots.emplace_back();
    }

    auto level = std::make_unique<LevelImpl>();
    level->handle.index = index;
    level->handle.generation = static_cast<std::uint32_t>(index + 1);
    level->guid = desc.guid.IsNil() ? WorldGuid::Generate() : desc.guid;
    CopyName(level->name, desc.name[0] ? desc.name : "Level");
    level->descriptor = desc;
    level->descriptor.guid = level->guid;
    level->owningWorld = m_Handle;
    level->persistent = desc.persistent;
    level->bounds.min = {desc.boundsMin[0], desc.boundsMin[1], desc.boundsMin[2]};
    level->bounds.max = {desc.boundsMax[0], desc.boundsMax[1], desc.boundsMax[2]};
    level->scene = std::make_shared<scene::Scene>();
    level->world = this;
    level->state = LevelState::Unloaded;

    LevelHandle handle = level->handle;
    m_LevelSlots[index] = std::move(level);
    m_LevelHandles.push_back(handle);
    WorldDiagnostics::Get().OnLevelCreated();
    return handle;
}

bool WorldImpl::DestroyLevel(LevelHandle level) {
    LevelImpl* impl = GetLevel(level);
    if (!impl || impl->persistent) {
        return false;
    }
    impl->Unload();
    m_LevelHandles.erase(std::remove(m_LevelHandles.begin(), m_LevelHandles.end(), level), m_LevelHandles.end());
    m_LevelSlots[level.index].reset();
    WorldDiagnostics::Get().OnLevelDestroyed();
    return true;
}

ILevel* WorldImpl::FindLevel(LevelHandle level) noexcept { return GetLevel(level); }
const ILevel* WorldImpl::FindLevel(LevelHandle level) const noexcept { return GetLevel(level); }

ILevel* WorldImpl::FindLevelByGuid(const WorldGuid& guid) noexcept {
    for (LevelHandle h : m_LevelHandles) {
        if (LevelImpl* l = GetLevel(h); l && l->guid == guid) {
            return l;
        }
    }
    return nullptr;
}

std::span<const LevelHandle> WorldImpl::Levels() const noexcept { return m_LevelHandles; }

scene::Scene* WorldImpl::ActiveScene() noexcept {
    LevelImpl* level = GetLevel(m_ActiveLevel.IsValid() ? m_ActiveLevel : m_PersistentLevel);
    return level ? level->scene.get() : nullptr;
}

const scene::Scene* WorldImpl::ActiveScene() const noexcept {
    return const_cast<WorldImpl*>(this)->ActiveScene();
}

ecs::Registry* WorldImpl::ActiveRegistry() noexcept {
    scene::Scene* scene = ActiveScene();
    return scene ? &scene->Registry() : nullptr;
}

ecs::World* WorldImpl::ActiveEcsWorld() noexcept {
    scene::Scene* scene = ActiveScene();
    return scene ? &scene->World() : nullptr;
}

ecs::SystemScheduler* WorldImpl::ActiveSystems() noexcept {
    scene::Scene* scene = ActiveScene();
    return scene ? &scene->Systems() : nullptr;
}

void WorldImpl::Tick(const WorldTickParams& params) {
    if (m_State != WorldState::Ready && m_State != WorldState::Streaming && m_State != WorldState::Saving) {
        return;
    }

    WorldTickParams local = params;
    local.worldTimeSeconds = m_Context.m_WorldTime + params.deltaSeconds;
    local.frameIndex = m_Context.m_Frame + 1;
    m_Context.SetTickParams(local);
    m_Context.m_WorldTime = local.worldTimeSeconds;
    m_Context.m_Frame = local.frameIndex;

    m_Origin.ApplyPendingRebase();
    m_Streamer.Tick(params.deltaSeconds);
    m_Partition.Tick(params.deltaSeconds);

    for (IWorldSubsystem* s : m_Subsystems.All()) {
        if (s) {
            s->Tick(m_Context, params.deltaSeconds);
        }
    }

    m_Ticks.Tick(m_Context, local);
    m_Transforms.UpdateWorldTransforms();

    for (LevelHandle h : m_LevelHandles) {
        if (LevelImpl* level = GetLevel(h)) {
            level->Tick(params.deltaSeconds);
        }
    }

    m_SceneGraph.Rebuild();
}

void WorldImpl::BeginPlay() {
    m_Context.m_Playing = true;
    m_Actors.FlushPendingBeginPlay();
    m_Subsystems.BeginPlayAll(m_Context);
}

void WorldImpl::EndPlay(EndPlayReason reason) {
    m_Subsystems.EndPlayAll(m_Context, reason);
    for (ActorHandle h : std::vector<ActorHandle>(m_Actors.All().begin(), m_Actors.All().end())) {
        m_Actors.Destroy(h, reason);
    }
    m_Context.m_Playing = false;
}

void WorldImpl::Suspend() {
    if (m_State == WorldState::Ready) {
        m_State = WorldState::Suspended;
    }
}

void WorldImpl::Resume() {
    if (m_State == WorldState::Suspended) {
        m_State = WorldState::Ready;
    }
}

void WorldImpl::Shutdown() {
    if (m_State == WorldState::Destroyed || m_State == WorldState::Destroying) {
        return;
    }
    m_State = WorldState::Destroying;
    EndPlay(EndPlayReason::WorldTeardown);
    m_Subsystems.DeinitializeAll();

    if (m_RuntimeDeps.physicsHook) m_RuntimeDeps.physicsHook->OnWorldDestroyed(*this);
    if (m_RuntimeDeps.audioHook) m_RuntimeDeps.audioHook->OnWorldDestroyed(*this);
    if (m_RuntimeDeps.networkHook) m_RuntimeDeps.networkHook->OnWorldDestroyed(*this);
    if (m_RuntimeDeps.editorHook) m_RuntimeDeps.editorHook->OnWorldDestroyed(*this);
    if (m_RuntimeDeps.renderHook) m_RuntimeDeps.renderHook->OnWorldDestroyed(*this);

    m_LevelHandles.clear();
    m_LevelSlots.clear();
    m_State = WorldState::Destroyed;
    WorldDiagnostics::Get().OnWorldDestroyed();
}

LevelImpl* WorldImpl::GetLevel(LevelHandle level) noexcept {
    if (!level.IsValid() || level.index >= m_LevelSlots.size() || !m_LevelSlots[level.index]) {
        return nullptr;
    }
    if (m_LevelSlots[level.index]->handle.generation != level.generation) {
        return nullptr;
    }
    return m_LevelSlots[level.index].get();
}

const LevelImpl* WorldImpl::GetLevel(LevelHandle level) const noexcept {
    return const_cast<WorldImpl*>(this)->GetLevel(level);
}

ActorImpl* WorldImpl::GetActor(ActorHandle handle) noexcept {
    return dynamic_cast<ActorImpl*>(m_Actors.TryGet(handle));
}

const ActorImpl* WorldImpl::GetActor(ActorHandle handle) const noexcept {
    return const_cast<WorldImpl*>(this)->GetActor(handle);
}

bool WorldRegistryImpl::Contains(WorldHandle handle) const noexcept {
    std::shared_lock lock(m_Mutex);
    return TryGet(handle) != nullptr;
}

bool WorldRegistryImpl::ContainsGuid(const WorldGuid& guid) const noexcept {
    std::shared_lock lock(m_Mutex);
    return m_ByGuid.contains(guid);
}

IWorld* WorldRegistryImpl::TryGet(WorldHandle handle) noexcept {
    if (!handle.IsValid() || handle.index >= m_Slots.size()) {
        return nullptr;
    }
    if (handle.index >= m_Generations.size() || m_Generations[handle.index] != handle.generation) {
        return nullptr;
    }
    return m_Slots[handle.index];
}

const IWorld* WorldRegistryImpl::TryGet(WorldHandle handle) const noexcept {
    return const_cast<WorldRegistryImpl*>(this)->TryGet(handle);
}

IWorld* WorldRegistryImpl::TryGetByGuid(const WorldGuid& guid) noexcept {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ByGuid.find(guid);
    return it == m_ByGuid.end() ? nullptr : TryGet(it->second);
}

IWorld* WorldRegistryImpl::TryGetByName(std::string_view name) noexcept {
    std::shared_lock lock(m_Mutex);
    for (WorldHandle h : m_All) {
        if (IWorld* w = TryGet(h); w && w->Name() == name) {
            return w;
        }
    }
    return nullptr;
}

std::span<const WorldHandle> WorldRegistryImpl::All() const noexcept { return m_All; }
std::size_t WorldRegistryImpl::Count() const noexcept { return m_All.size(); }

void WorldRegistryImpl::Add(WorldHandle handle, WorldImpl* world) {
    std::unique_lock lock(m_Mutex);
    if (handle.index >= m_Slots.size()) {
        m_Slots.resize(handle.index + 1, nullptr);
        m_Generations.resize(handle.index + 1, 0);
    }
    m_Slots[handle.index] = world;
    m_Generations[handle.index] = handle.generation;
    m_All.push_back(handle);
    m_ByGuid[world->Guid()] = handle;
}

void WorldRegistryImpl::Remove(WorldHandle handle) {
    std::unique_lock lock(m_Mutex);
    if (IWorld* w = TryGet(handle)) {
        m_ByGuid.erase(w->Guid());
    }
    if (handle.index < m_Slots.size()) {
        m_Slots[handle.index] = nullptr;
        m_Generations[handle.index] = 0;
    }
    m_All.erase(std::remove(m_All.begin(), m_All.end(), handle), m_All.end());
}

WorldManagerImpl::WorldManagerImpl(WorldRuntimeDependencies deps) : m_Deps(std::move(deps)) {}

WorldManagerImpl::~WorldManagerImpl() {
    Shutdown();
}

WorldHandle WorldManagerImpl::CreateWorld(const WorldCreateInfo& info) {
    std::lock_guard lock(m_Mutex);
    const std::uint32_t index = m_NextIndex++;
    WorldHandle handle{index, index + 1};

    auto world = std::make_unique<WorldImpl>(handle, info, this, m_Deps);
    WorldImpl* raw = world.get();
    m_Worlds.push_back(std::move(world));
    m_Registry.Add(handle, raw);
    if (!m_Active.IsValid()) {
        m_Active = handle;
    }
    return handle;
}

bool WorldManagerImpl::DestroyWorld(WorldHandle handle) {
    std::lock_guard lock(m_Mutex);
    IWorld* world = m_Registry.TryGet(handle);
    if (!world) {
        return false;
    }
    world->Shutdown();
    m_Registry.Remove(handle);
    m_Worlds.erase(
        std::remove_if(m_Worlds.begin(), m_Worlds.end(),
            [handle](const std::unique_ptr<WorldImpl>& w) {
                return w && w->Handle() == handle;
            }),
        m_Worlds.end());
    if (m_Active == handle) {
        m_Active = m_Registry.All().empty() ? WorldHandle{} : m_Registry.All().front();
    }
    return true;
}

IWorld* WorldManagerImpl::FindWorld(WorldHandle handle) noexcept { return m_Registry.TryGet(handle); }
const IWorld* WorldManagerImpl::FindWorld(WorldHandle handle) const noexcept { return m_Registry.TryGet(handle); }
IWorld* WorldManagerImpl::FindWorldByGuid(const WorldGuid& guid) noexcept { return m_Registry.TryGetByGuid(guid); }
IWorld* WorldManagerImpl::FindWorldByName(std::string_view name) noexcept { return m_Registry.TryGetByName(name); }
std::span<const WorldHandle> WorldManagerImpl::Worlds() const noexcept { return m_Registry.All(); }

void WorldManagerImpl::SetActiveWorld(WorldHandle handle) {
    if (m_Registry.Contains(handle)) {
        m_Active = handle;
    }
}

void WorldManagerImpl::TickAll(const WorldTickParams& params) {
    for (WorldHandle h : std::vector<WorldHandle>(m_Registry.All().begin(), m_Registry.All().end())) {
        TickWorld(h, params);
    }
}

void WorldManagerImpl::TickWorld(WorldHandle handle, const WorldTickParams& params) {
    if (IWorld* w = FindWorld(handle)) {
        w->Tick(params);
    }
}

void WorldManagerImpl::Shutdown() {
    std::lock_guard lock(m_Mutex);
    auto handles = std::vector<WorldHandle>(m_Registry.All().begin(), m_Registry.All().end());
    for (WorldHandle h : handles) {
        if (IWorld* w = m_Registry.TryGet(h)) {
            w->Shutdown();
        }
        m_Registry.Remove(h);
    }
    m_Worlds.clear();
    m_Active = {};
}

} // namespace we::runtime::world::detail
