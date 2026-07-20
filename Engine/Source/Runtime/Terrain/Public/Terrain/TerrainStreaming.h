#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainChunkManager.h"

#include <algorithm>
#include <unordered_set>

namespace we::runtime::terrain {

// Chunk residency / async load hooks for large open worlds.
// Today: frustum culling + active set. Future: streamed height/weight pages + VT.
class TERRAIN_API TerrainStreaming {
public:
    void SetLoadRadiusChunks(int radius) { m_LoadRadius = (std::max)(0, radius); }
    int LoadRadiusChunks() const { return m_LoadRadius; }
    void SetMaxResidentChunks(int count) { m_MaxResident = (std::max)(1, count); }
    int MaxResidentChunks() const { return m_MaxResident; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    /// Updates residency. Never translates chunk world positions — only visibility/residency.
    /// When the full terrain fits in maxResident, all chunks stay resident (fixed world actor).
    void Update(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
        const TerrainCreateInfo& info, const TerrainFrustump* frustum);

    const std::unordered_set<TerrainChunkId, TerrainChunkIdHash>& ActiveChunks() const {
        return m_Active;
    }

    // Extension points for world composition.
    void RequestLoad(TerrainChunkId id);
    void RequestUnload(TerrainChunkId id);

private:
    int m_LoadRadius = 8;
    int m_MaxResident = 64;
    bool m_Enabled = true;
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_Active;
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_PendingLoad;
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_PendingUnload;
};

} // namespace we::runtime::terrain

