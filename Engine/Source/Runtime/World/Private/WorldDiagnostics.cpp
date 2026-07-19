#include "World/WorldDiagnostics.h"

#include <sstream>

namespace we::runtime::world {

WorldDiagnostics& WorldDiagnostics::Get() noexcept {
    static WorldDiagnostics instance;
    return instance;
}

void WorldDiagnostics::Reset() noexcept {
    m_WorldCount.store(0, std::memory_order_relaxed);
    m_LevelCount.store(0, std::memory_order_relaxed);
    m_ActorCount.store(0, std::memory_order_relaxed);
    m_SpawnedActors.store(0, std::memory_order_relaxed);
    m_DestroyedActors.store(0, std::memory_order_relaxed);
    m_TickCount.store(0, std::memory_order_relaxed);
    m_FixedTickCount.store(0, std::memory_order_relaxed);
    m_BeginPlayCount.store(0, std::memory_order_relaxed);
    m_EndPlayCount.store(0, std::memory_order_relaxed);
    m_StreamLoads.store(0, std::memory_order_relaxed);
    m_StreamUnloads.store(0, std::memory_order_relaxed);
    m_SaveOperations.store(0, std::memory_order_relaxed);
    m_LoadOperations.store(0, std::memory_order_relaxed);
    m_HierarchyOps.store(0, std::memory_order_relaxed);
    m_QueryOps.store(0, std::memory_order_relaxed);
    m_SpatialQueryOps.store(0, std::memory_order_relaxed);
    m_LastTickNs.store(0, std::memory_order_relaxed);
}

WorldDiagnosticsSnapshot WorldDiagnostics::Capture() const noexcept {
    WorldDiagnosticsSnapshot snap{};
    snap.worldCount = m_WorldCount.load(std::memory_order_relaxed);
    snap.levelCount = m_LevelCount.load(std::memory_order_relaxed);
    snap.actorCount = m_ActorCount.load(std::memory_order_relaxed);
    snap.spawnedActors = m_SpawnedActors.load(std::memory_order_relaxed);
    snap.destroyedActors = m_DestroyedActors.load(std::memory_order_relaxed);
    snap.tickCount = m_TickCount.load(std::memory_order_relaxed);
    snap.fixedTickCount = m_FixedTickCount.load(std::memory_order_relaxed);
    snap.beginPlayCount = m_BeginPlayCount.load(std::memory_order_relaxed);
    snap.endPlayCount = m_EndPlayCount.load(std::memory_order_relaxed);
    snap.streamLoads = m_StreamLoads.load(std::memory_order_relaxed);
    snap.streamUnloads = m_StreamUnloads.load(std::memory_order_relaxed);
    snap.saveOperations = m_SaveOperations.load(std::memory_order_relaxed);
    snap.loadOperations = m_LoadOperations.load(std::memory_order_relaxed);
    snap.hierarchyOps = m_HierarchyOps.load(std::memory_order_relaxed);
    snap.queryOps = m_QueryOps.load(std::memory_order_relaxed);
    snap.spatialQueryOps = m_SpatialQueryOps.load(std::memory_order_relaxed);
    snap.lastTickMilliseconds = static_cast<double>(m_LastTickNs.load(std::memory_order_relaxed)) / 1.0e6;
    return snap;
}

void WorldDiagnostics::OnWorldCreated() noexcept { m_WorldCount.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnWorldDestroyed() noexcept {
    auto v = m_WorldCount.load(std::memory_order_relaxed);
    if (v > 0) m_WorldCount.store(v - 1, std::memory_order_relaxed);
}
void WorldDiagnostics::OnLevelCreated() noexcept { m_LevelCount.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnLevelDestroyed() noexcept {
    auto v = m_LevelCount.load(std::memory_order_relaxed);
    if (v > 0) m_LevelCount.store(v - 1, std::memory_order_relaxed);
}
void WorldDiagnostics::OnActorSpawned() noexcept {
    m_ActorCount.fetch_add(1, std::memory_order_relaxed);
    m_SpawnedActors.fetch_add(1, std::memory_order_relaxed);
}
void WorldDiagnostics::OnActorDestroyed() noexcept {
    auto v = m_ActorCount.load(std::memory_order_relaxed);
    if (v > 0) m_ActorCount.store(v - 1, std::memory_order_relaxed);
    m_DestroyedActors.fetch_add(1, std::memory_order_relaxed);
}
void WorldDiagnostics::OnTick(double milliseconds) noexcept {
    m_TickCount.fetch_add(1, std::memory_order_relaxed);
    m_LastTickNs.store(static_cast<std::uint64_t>(milliseconds * 1.0e6), std::memory_order_relaxed);
}
void WorldDiagnostics::OnFixedTick() noexcept { m_FixedTickCount.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnBeginPlay() noexcept { m_BeginPlayCount.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnEndPlay() noexcept { m_EndPlayCount.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnStreamLoad() noexcept { m_StreamLoads.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnStreamUnload() noexcept { m_StreamUnloads.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnSave() noexcept { m_SaveOperations.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnLoad() noexcept { m_LoadOperations.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnHierarchyOp() noexcept { m_HierarchyOps.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnQuery() noexcept { m_QueryOps.fetch_add(1, std::memory_order_relaxed); }
void WorldDiagnostics::OnSpatialQuery() noexcept { m_SpatialQueryOps.fetch_add(1, std::memory_order_relaxed); }

std::string FormatWorldDiagnostics(const WorldDiagnosticsSnapshot& snap) {
    std::ostringstream oss;
    oss << "WorldDiagnostics{worlds=" << snap.worldCount
        << " levels=" << snap.levelCount
        << " actors=" << snap.actorCount
        << " ticks=" << snap.tickCount
        << " lastTickMs=" << snap.lastTickMilliseconds << '}';
    return oss.str();
}

} // namespace we::runtime::world
