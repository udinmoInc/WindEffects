#include "Prefab/PrefabDiagnostics.h"

#include <sstream>

namespace we::runtime::prefab {

PrefabDiagnostics& PrefabDiagnostics::Get() noexcept {
    static PrefabDiagnostics instance;
    return instance;
}

void PrefabDiagnostics::Reset() noexcept {
    m_Assets.store(0);
    m_Spawned.store(0);
    m_Alive.store(0);
    m_Overrides.store(0);
    m_Apply.store(0);
    m_SpawnMicros.store(0);
    m_SerializeMicros.store(0);
}

PrefabDiagnosticsSnapshot PrefabDiagnostics::Snapshot() const noexcept {
    PrefabDiagnosticsSnapshot s;
    s.assetsRegistered = m_Assets.load();
    s.instancesSpawned = m_Spawned.load();
    s.instancesAlive = m_Alive.load();
    s.overridesRecorded = m_Overrides.load();
    s.applyCount = m_Apply.load();
    s.spawnMicros = m_SpawnMicros.load();
    s.serializeMicros = m_SerializeMicros.load();
    return s;
}

std::string PrefabDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "Prefab assets=" << s.assetsRegistered
        << " spawned=" << s.instancesSpawned
        << " alive=" << s.instancesAlive
        << " overrides=" << s.overridesRecorded
        << " apply=" << s.applyCount
        << " spawnUs=" << s.spawnMicros
        << " serializeUs=" << s.serializeMicros;
    return oss.str();
}

void PrefabDiagnostics::OnAssetRegistered() noexcept { m_Assets.fetch_add(1); }

void PrefabDiagnostics::OnSpawn(std::uint64_t micros) noexcept {
    m_Spawned.fetch_add(1);
    m_Alive.fetch_add(1);
    m_SpawnMicros.fetch_add(micros);
}

void PrefabDiagnostics::OnDespawn() noexcept {
    if (m_Alive.load() > 0) {
        m_Alive.fetch_sub(1);
    }
}

void PrefabDiagnostics::OnOverride() noexcept { m_Overrides.fetch_add(1); }
void PrefabDiagnostics::OnApply() noexcept { m_Apply.fetch_add(1); }

void PrefabDiagnostics::OnSerialize(std::uint64_t micros) noexcept {
    m_SerializeMicros.fetch_add(micros);
}

} // namespace we::runtime::prefab
