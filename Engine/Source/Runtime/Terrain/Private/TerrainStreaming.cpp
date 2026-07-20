#include "Terrain/TerrainStreaming.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

void TerrainStreaming::RequestLoad(TerrainChunkId id) {
    m_PendingUnload.erase(id);
    m_PendingLoad.insert(id);
}

void TerrainStreaming::RequestUnload(TerrainChunkId id) {
    m_PendingLoad.erase(id);
    m_PendingUnload.insert(id);
}

void TerrainStreaming::Update(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
    const TerrainCreateInfo& info, const TerrainFrustump* frustum) {
    if (!m_Enabled || chunks.Chunks().empty()) {
        return;
    }

    m_Active.clear();

    // Small/default landscapes fit entirely in the resident budget — keep every chunk
    // resident so camera motion never looks like an infinite plane sliding under the view.
    const bool keepAllResident =
        static_cast<int>(chunks.Chunks().size()) <= m_MaxResident;

    const float localX = cameraWorldPos.x - info.worldOrigin.x;
    const float localZ = cameraWorldPos.z - info.worldOrigin.z;
    const TerrainChunkId cameraChunk = chunks.WorldToChunkId(localX, localZ, info);

    for (TerrainChunkId id : m_PendingLoad) {
        m_Active.insert(id);
    }

    for (TerrainChunk& chunk : chunks.Chunks()) {
        bool resident = keepAllResident || m_Active.contains(chunk.id);
        if (!resident) {
            const int dx = chunk.id.x - cameraChunk.x;
            const int dz = chunk.id.z - cameraChunk.z;
            resident = (dx * dx + dz * dz) <= (m_LoadRadius * m_LoadRadius);
        }

        bool inFrustum = true;
        if (frustum && chunk.bounds.min.x <= chunk.bounds.max.x) {
            inFrustum = chunk.bounds.IntersectsFrustumPlanes(frustum->planes);
        }

        // Residency stays independent of frustum; frustum only gates draw.
        chunk.visible = resident && inFrustum;
        if (resident) {
            m_Active.insert(chunk.id);
        }
    }
    m_PendingLoad.clear();

    for (TerrainChunkId id : m_PendingUnload) {
        if (keepAllResident) {
            continue; // refuse unload while the whole landscape is budget-resident
        }
        if (TerrainChunk* chunk = chunks.Find(id)) {
            chunk->visible = false;
            m_Active.erase(id);
        }
    }
    m_PendingUnload.clear();
}

} // namespace we::runtime::terrain
