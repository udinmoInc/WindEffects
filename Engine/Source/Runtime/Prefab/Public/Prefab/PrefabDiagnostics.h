#pragma once

#include "Prefab/Export.h"
#include "Prefab/PrefabTypes.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::runtime::prefab {

struct PREFAB_API PrefabDiagnosticsSnapshot {
    std::uint64_t assetsRegistered = 0;
    std::uint64_t instancesSpawned = 0;
    std::uint64_t instancesAlive = 0;
    std::uint64_t overridesRecorded = 0;
    std::uint64_t applyCount = 0;
    std::uint64_t spawnMicros = 0;
    std::uint64_t serializeMicros = 0;
};

class PREFAB_API PrefabDiagnostics {
public:
    static PrefabDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] PrefabDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnAssetRegistered() noexcept;
    void OnSpawn(std::uint64_t micros) noexcept;
    void OnDespawn() noexcept;
    void OnOverride() noexcept;
    void OnApply() noexcept;
    void OnSerialize(std::uint64_t micros) noexcept;

private:
    std::atomic<std::uint64_t> m_Assets{0};
    std::atomic<std::uint64_t> m_Spawned{0};
    std::atomic<std::uint64_t> m_Alive{0};
    std::atomic<std::uint64_t> m_Overrides{0};
    std::atomic<std::uint64_t> m_Apply{0};
    std::atomic<std::uint64_t> m_SpawnMicros{0};
    std::atomic<std::uint64_t> m_SerializeMicros{0};
};

} // namespace we::runtime::prefab
