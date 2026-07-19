#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"

#include <cstdint>
#include <vector>

namespace we::runtime::terrain {

// Per-chunk state. Mesh regeneration is independent — dirty chunks rebuild alone.
struct TerrainChunk {
    TerrainChunkId id{};
    int vertexOriginX = 0; // heightmap sample origin
    int vertexOriginZ = 0;
    int quads = kDefaultChunkQuads;
    int lod = 0;
    bool visible = true;
    bool meshDirty = true;
    bool gpuDirty = true;
    bool collisionDirty = true;
    TerrainAABB bounds{};
    TerrainMeshCPU mesh{};
    // Optional painted weight subset for this chunk (RGBA, same resolution as verts).
    std::vector<std::uint8_t> weightRGBA;
};

class TERRAIN_API TerrainChunkManager {
public:
    void Initialize(const TerrainCreateInfo& info);
    void Shutdown();

    int ChunkCountX() const { return m_ChunksX; }
    int ChunkCountZ() const { return m_ChunksZ; }
    int ChunkQuads() const { return m_ChunkQuads; }
    int ChunkVerts() const { return m_ChunkQuads + 1; }

    TerrainChunk* Find(TerrainChunkId id);
    const TerrainChunk* Find(TerrainChunkId id) const;
    TerrainChunk* At(int cx, int cz);
    const TerrainChunk* At(int cx, int cz) const;

    std::vector<TerrainChunk>& Chunks() { return m_Chunks; }
    const std::vector<TerrainChunk>& Chunks() const { return m_Chunks; }

    TerrainChunkId WorldToChunkId(float localX, float localZ, const TerrainCreateInfo& info) const;
    void MarkDirtyRect(int minX, int minZ, int maxX, int maxZ);
    void MarkAllDirty();
    int RebuildDirtyMeshes(const class TerrainHeightmap& heightmap, const TerrainCreateInfo& info,
        class TerrainLODManager& lod);

private:
    int IndexOf(int cx, int cz) const { return cz * m_ChunksX + cx; }

    int m_ChunksX = 0;
    int m_ChunksZ = 0;
    int m_ChunkQuads = kDefaultChunkQuads;
    std::vector<TerrainChunk> m_Chunks;
};

} // namespace we::runtime::terrain

