#pragma once

#include "World/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::runtime::world {

struct WORLD_API WorldDiagnosticsSnapshot {
    std::uint64_t worldCount = 0;
    std::uint64_t levelCount = 0;
    std::uint64_t actorCount = 0;
    std::uint64_t spawnedActors = 0;
    std::uint64_t destroyedActors = 0;
    std::uint64_t tickCount = 0;
    std::uint64_t fixedTickCount = 0;
    std::uint64_t beginPlayCount = 0;
    std::uint64_t endPlayCount = 0;
    std::uint64_t streamLoads = 0;
    std::uint64_t streamUnloads = 0;
    std::uint64_t saveOperations = 0;
    std::uint64_t loadOperations = 0;
    std::uint64_t hierarchyOps = 0;
    std::uint64_t queryOps = 0;
    std::uint64_t spatialQueryOps = 0;
    std::uint64_t allocationBytesApprox = 0;
    double lastTickMilliseconds = 0.0;
};

/// Process-local atomic counters for production diagnostics / profiling hooks.
class WORLD_API WorldDiagnostics {
public:
    static WorldDiagnostics& Get() noexcept;

    void Reset() noexcept;
    [[nodiscard]] WorldDiagnosticsSnapshot Capture() const noexcept;

    void OnWorldCreated() noexcept;
    void OnWorldDestroyed() noexcept;
    void OnLevelCreated() noexcept;
    void OnLevelDestroyed() noexcept;
    void OnActorSpawned() noexcept;
    void OnActorDestroyed() noexcept;
    void OnTick(double milliseconds) noexcept;
    void OnFixedTick() noexcept;
    void OnBeginPlay() noexcept;
    void OnEndPlay() noexcept;
    void OnStreamLoad() noexcept;
    void OnStreamUnload() noexcept;
    void OnSave() noexcept;
    void OnLoad() noexcept;
    void OnHierarchyOp() noexcept;
    void OnQuery() noexcept;
    void OnSpatialQuery() noexcept;

private:
    WorldDiagnostics() = default;

    std::atomic<std::uint64_t> m_WorldCount{0};
    std::atomic<std::uint64_t> m_LevelCount{0};
    std::atomic<std::uint64_t> m_ActorCount{0};
    std::atomic<std::uint64_t> m_SpawnedActors{0};
    std::atomic<std::uint64_t> m_DestroyedActors{0};
    std::atomic<std::uint64_t> m_TickCount{0};
    std::atomic<std::uint64_t> m_FixedTickCount{0};
    std::atomic<std::uint64_t> m_BeginPlayCount{0};
    std::atomic<std::uint64_t> m_EndPlayCount{0};
    std::atomic<std::uint64_t> m_StreamLoads{0};
    std::atomic<std::uint64_t> m_StreamUnloads{0};
    std::atomic<std::uint64_t> m_SaveOperations{0};
    std::atomic<std::uint64_t> m_LoadOperations{0};
    std::atomic<std::uint64_t> m_HierarchyOps{0};
    std::atomic<std::uint64_t> m_QueryOps{0};
    std::atomic<std::uint64_t> m_SpatialQueryOps{0};
    std::atomic<std::uint64_t> m_LastTickNs{0};
};

[[nodiscard]] WORLD_API std::string FormatWorldDiagnostics(const WorldDiagnosticsSnapshot& snap);

} // namespace we::runtime::world
