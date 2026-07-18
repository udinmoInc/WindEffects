#include "Terrain/TerrainStreaming.h"

#include <algorithm>
#include <cmath>

#include "Core/Math/GlmInterop.h"
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
    if (chunks.Chunks().empty()) {
        return;
    }

    const float localX = cameraWorldPos.x - info.worldOrigin.x;
    const float localZ = cameraWorldPos.z - info.worldOrigin.z;
    const TerrainChunkId cameraChunk = chunks.WorldToChunkId(localX, localZ, info);

    m_Active.clear();
    for (TerrainChunk& chunk : chunks.Chunks()) {
        const int dx = chunk.id.x - cameraChunk.x;
        const int dz = chunk.id.z - cameraChunk.z;
        const bool inRadius = (dx * dx + dz * dz) <= (m_LoadRadius * m_LoadRadius);

        bool inFrustum = true;
        if (frustum) {
            inFrustum = chunk.bounds.IntersectsFrustumPlanes(frustum->planes);
        }

        chunk.visible = inRadius && inFrustum;
        if (chunk.visible) {
            m_Active.insert(chunk.id);
        }
    }

    for (TerrainChunkId id : m_PendingLoad) {
        if (TerrainChunk* chunk = chunks.Find(id)) {
            chunk->visible = true;
            m_Active.insert(id);
        }
    }
    m_PendingLoad.clear();

    for (TerrainChunkId id : m_PendingUnload) {
        if (TerrainChunk* chunk = chunks.Find(id)) {
            chunk->visible = false;
            m_Active.erase(id);
        }
    }
    m_PendingUnload.clear();
}

} // namespace we::runtime::terrain
