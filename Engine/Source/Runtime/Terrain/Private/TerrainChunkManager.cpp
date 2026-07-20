#include "Terrain/TerrainChunkManager.h"
#include "Terrain/TerrainHeightmap.h"
#include "Terrain/TerrainLODManager.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

void TerrainChunkManager::Initialize(const TerrainCreateInfo& info) {
    m_ChunkQuads = std::max(1, info.chunkQuads);
    const int samplesX = std::max(2, info.resolutionX);
    const int samplesZ = std::max(2, info.resolutionY);
    m_ChunksX = std::max(1, (samplesX - 1) / m_ChunkQuads);
    m_ChunksZ = std::max(1, (samplesZ - 1) / m_ChunkQuads);
    m_Chunks.clear();
    m_Chunks.resize(static_cast<std::size_t>(m_ChunksX * m_ChunksZ));

    const float dx = (info.worldSizeX * info.worldScale.x)
        / static_cast<float>(std::max(1, samplesX - 1));
    const float dz = (info.worldSizeY * info.worldScale.z)
        / static_cast<float>(std::max(1, samplesZ - 1));
    const float y0 = info.heightOffset;
    const float y1 = info.heightOffset + info.heightScale * info.worldScale.y;

    for (int cz = 0; cz < m_ChunksZ; ++cz) {
        for (int cx = 0; cx < m_ChunksX; ++cx) {
            TerrainChunk& chunk = m_Chunks[static_cast<std::size_t>(IndexOf(cx, cz))];
            chunk.id = { cx, cz };
            chunk.vertexOriginX = cx * m_ChunkQuads;
            chunk.vertexOriginZ = cz * m_ChunkQuads;
            chunk.quads = m_ChunkQuads;
            chunk.lod = 0;
            chunk.visible = true;
            chunk.meshDirty = true;
            chunk.gpuDirty = true;
            chunk.collisionDirty = true;
            // Approximate world AABB so LOD/frustum work before the first remesh.
            const float x0 = info.worldOrigin.x + static_cast<float>(chunk.vertexOriginX) * dx;
            const float z0 = info.worldOrigin.z + static_cast<float>(chunk.vertexOriginZ) * dz;
            const float x1 = info.worldOrigin.x
                + static_cast<float>(std::min(chunk.vertexOriginX + chunk.quads, samplesX - 1)) * dx;
            const float z1 = info.worldOrigin.z
                + static_cast<float>(std::min(chunk.vertexOriginZ + chunk.quads, samplesZ - 1)) * dz;
            chunk.bounds.min = we::math::Vec3(x0, y0, z0);
            chunk.bounds.max = we::math::Vec3(x1, y1, z1);
        }
    }
}

void TerrainChunkManager::Shutdown() {
    m_Chunks.clear();
    m_ChunksX = 0;
    m_ChunksZ = 0;
}

TerrainChunk* TerrainChunkManager::Find(TerrainChunkId id) {
    return At(id.x, id.z);
}

const TerrainChunk* TerrainChunkManager::Find(TerrainChunkId id) const {
    return At(id.x, id.z);
}

TerrainChunk* TerrainChunkManager::At(int cx, int cz) {
    if (cx < 0 || cz < 0 || cx >= m_ChunksX || cz >= m_ChunksZ) {
        return nullptr;
    }
    return &m_Chunks[static_cast<std::size_t>(IndexOf(cx, cz))];
}

const TerrainChunk* TerrainChunkManager::At(int cx, int cz) const {
    if (cx < 0 || cz < 0 || cx >= m_ChunksX || cz >= m_ChunksZ) {
        return nullptr;
    }
    return &m_Chunks[static_cast<std::size_t>(IndexOf(cx, cz))];
}

TerrainChunkId TerrainChunkManager::WorldToChunkId(float localX, float localZ,
    const TerrainCreateInfo& info) const {
    const float u = (info.worldSizeX > 1e-6f) ? (localX / info.worldSizeX) : 0.0f;
    const float v = (info.worldSizeY > 1e-6f) ? (localZ / info.worldSizeY) : 0.0f;
    const int sampleX = static_cast<int>(std::floor(u * static_cast<float>(info.resolutionX - 1)));
    const int sampleZ = static_cast<int>(std::floor(v * static_cast<float>(info.resolutionY - 1)));
    TerrainChunkId id{};
    id.x = std::clamp(sampleX / m_ChunkQuads, 0, std::max(0, m_ChunksX - 1));
    id.z = std::clamp(sampleZ / m_ChunkQuads, 0, std::max(0, m_ChunksZ - 1));
    return id;
}

void TerrainChunkManager::MarkDirtyRect(int minX, int minZ, int maxX, int maxZ) {
    if (m_Chunks.empty()) {
        return;
    }
    // Expand by one sample so shared edges / normals of neighboring chunks update.
    minX = std::max(0, minX - 1);
    minZ = std::max(0, minZ - 1);
    maxX = maxX + 1;
    maxZ = maxZ + 1;
    const int cMinX = std::clamp(minX / m_ChunkQuads, 0, m_ChunksX - 1);
    const int cMaxX = std::clamp(maxX / m_ChunkQuads, 0, m_ChunksX - 1);
    const int cMinZ = std::clamp(minZ / m_ChunkQuads, 0, m_ChunksZ - 1);
    const int cMaxZ = std::clamp(maxZ / m_ChunkQuads, 0, m_ChunksZ - 1);
    for (int cz = cMinZ; cz <= cMaxZ; ++cz) {
        for (int cx = cMinX; cx <= cMaxX; ++cx) {
            if (TerrainChunk* chunk = At(cx, cz)) {
                chunk->meshDirty = true;
                chunk->collisionDirty = true;
            }
        }
    }
}

void TerrainChunkManager::MarkAllDirty() {
    for (TerrainChunk& chunk : m_Chunks) {
        chunk.meshDirty = true;
        chunk.gpuDirty = true;
        chunk.collisionDirty = true;
    }
}

int TerrainChunkManager::RebuildDirtyMeshes(const TerrainHeightmap& heightmap,
    const TerrainCreateInfo& info, TerrainLODManager& lod) {
    int rebuilt = 0;
    for (TerrainChunk& chunk : m_Chunks) {
        if (!chunk.meshDirty) {
            continue;
        }
        if (TerrainLODManager::BuildChunkMesh(heightmap, info, chunk, chunk.lod, chunk.mesh)) {
            chunk.meshDirty = false;
            chunk.gpuDirty = true;
            ++rebuilt;
        }
    }
    (void)lod;
    return rebuilt;
}

} // namespace we::runtime::terrain
