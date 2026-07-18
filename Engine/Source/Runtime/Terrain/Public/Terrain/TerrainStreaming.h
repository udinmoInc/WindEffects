#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainChunkManager.h"

#include <unordered_set>

namespace we::runtime::terrain {

// Chunk residency / async load hooks for large open worlds.
// Today: frustum culling + active set. Future: streamed height/weight pages + VT.
class TERRAIN_API TerrainStreaming {
public:
    void SetLoadRadiusChunks(int radius) { m_LoadRadius = std::max(0, radius); }
    int LoadRadiusChunks() const { return m_LoadRadius; }

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
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_Active;
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_PendingLoad;
    std::unordered_set<TerrainChunkId, TerrainChunkIdHash> m_PendingUnload;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
