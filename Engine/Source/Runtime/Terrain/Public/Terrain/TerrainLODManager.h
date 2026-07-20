#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainChunkManager.h"

#include <algorithm>
#include <vector>

namespace we::runtime::terrain {

class TerrainHeightmap;

// Selects per-chunk LOD from camera distance / screen size heuristics.
class TERRAIN_API TerrainLODManager {
public:
    void SetMaxLod(int maxLod) { m_MaxLod = (std::max)(0, (std::min)(maxLod, kMaxLodLevels - 1)); }
    void SetLodBias(float bias) { m_LodBias = (std::max)(0.25f, bias); }
    int MaxLod() const { return m_MaxLod; }

    // LOD 0 = full resolution. Each step roughly halves sampling density.
    int SelectLod(const TerrainChunk& chunk, const we::math::Vec3& cameraWorldPos,
        float worldSizeX, float worldSizeY, int heightmapWidth) const;

    void UpdateChunkLods(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
        const TerrainCreateInfo& info, int heightmapWidth);

    /// Force LOD from camera with no hysteresis (create / resize).
    void SeedChunkLods(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
        const TerrainCreateInfo& info, int heightmapWidth);

    // Build CPU mesh for a chunk at the requested LOD from the heightmap.
    static bool BuildChunkMesh(const TerrainHeightmap& heightmap, const TerrainCreateInfo& info,
        const TerrainChunk& chunk, int lod, TerrainMeshCPU& outMesh);

private:
    int m_MaxLod = 4;
    float m_LodBias = 1.75f;
};

} // namespace we::runtime::terrain

