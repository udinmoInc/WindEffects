#include "Terrain/TerrainLODManager.h"
#include "Terrain/TerrainHeightmap.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::terrain {

namespace {

float SampleHeightMeters(const TerrainHeightmap& map, const TerrainCreateInfo& info, int x, int z) {
    const float n = static_cast<float>(map.Get(x, z)) / 65535.0f;
    return info.heightOffset + n * info.heightScale;
}

glm::vec3 ComputeNormal(const TerrainHeightmap& map, const TerrainCreateInfo& info, int x, int z) {
    const int x0 = std::max(0, x - 1);
    const int x1 = std::min(map.Width() - 1, x + 1);
    const int z0 = std::max(0, z - 1);
    const int z1 = std::min(map.Height() - 1, z + 1);
    const float hL = SampleHeightMeters(map, info, x0, z);
    const float hR = SampleHeightMeters(map, info, x1, z);
    const float hD = SampleHeightMeters(map, info, x, z0);
    const float hU = SampleHeightMeters(map, info, x, z1);
    const float dx = info.worldSizeX / static_cast<float>(std::max(1, map.Width() - 1));
    const float dz = info.worldSizeY / static_cast<float>(std::max(1, map.Height() - 1));
    glm::vec3 n(-(hR - hL) / (dx * static_cast<float>(x1 - x0 == 0 ? 1 : (x1 - x0))),
        2.0f,
        -(hU - hD) / (dz * static_cast<float>(z1 - z0 == 0 ? 1 : (z1 - z0))));
    const float len = glm::length(n);
    return (len > 1e-6f) ? (n / len) : glm::vec3(0.0f, 1.0f, 0.0f);
}

} // namespace

int TerrainLODManager::SelectLod(const TerrainChunk& chunk, const glm::vec3& cameraWorldPos,
    float worldSizeX, float worldSizeY, int heightmapWidth) const {
    (void)worldSizeY;
    (void)heightmapWidth;
    const glm::vec3 center = (chunk.bounds.min + chunk.bounds.max) * 0.5f;
    const float dist = glm::length(cameraWorldPos - center);
    const float chunkWorld = worldSizeX / static_cast<float>(std::max(1, chunk.quads > 0 ? 8 : 1));
    const float relative = dist / std::max(1.0f, chunkWorld);
    int lod = 0;
    if (relative > 4.0f) lod = 1;
    if (relative > 8.0f) lod = 2;
    if (relative > 16.0f) lod = 3;
    if (relative > 32.0f) lod = 4;
    return std::min(lod, m_MaxLod);
}

void TerrainLODManager::UpdateChunkLods(TerrainChunkManager& chunks, const glm::vec3& cameraWorldPos,
    const TerrainCreateInfo& info, int heightmapWidth) {
    for (TerrainChunk& chunk : chunks.Chunks()) {
        const int newLod = SelectLod(chunk, cameraWorldPos, info.worldSizeX, info.worldSizeY, heightmapWidth);
        if (newLod != chunk.lod) {
            chunk.lod = newLod;
            chunk.meshDirty = true;
        }
    }
}

bool TerrainLODManager::BuildChunkMesh(const TerrainHeightmap& heightmap, const TerrainCreateInfo& info,
    const TerrainChunk& chunk, int lod, TerrainMeshCPU& outMesh) {
    if (heightmap.Empty()) {
        return false;
    }

    const int step = 1 << std::clamp(lod, 0, kMaxLodLevels - 1);
    const int verts = chunk.quads + 1;
    // Ensure we stay within heightmap and land on the far edge of the chunk.
    if ((verts - 1) % step != 0) {
        // For non-divisible LOD, clamp step down until divisible.
        int s = step;
        while (s > 1 && ((verts - 1) % s) != 0) {
            s >>= 1;
        }
        lod = 0;
        while ((1 << lod) < s) {
            ++lod;
        }
    }
    const int stride = 1 << std::clamp(lod, 0, kMaxLodLevels - 1);
    const int grid = ((verts - 1) / stride) + 1;
    if (grid < 2) {
        return false;
    }

    outMesh.positions.clear();
    outMesh.normals.clear();
    outMesh.uvs.clear();
    outMesh.indices.clear();
    outMesh.positions.reserve(static_cast<std::size_t>(grid * grid));
    outMesh.normals.reserve(static_cast<std::size_t>(grid * grid));
    outMesh.uvs.reserve(static_cast<std::size_t>(grid * grid));
    outMesh.indices.reserve(static_cast<std::size_t>((grid - 1) * (grid - 1) * 6));

    const float dx = info.worldSizeX / static_cast<float>(std::max(1, heightmap.Width() - 1));
    const float dz = info.worldSizeY / static_cast<float>(std::max(1, heightmap.Height() - 1));

    glm::vec3 bmin(1e30f);
    glm::vec3 bmax(-1e30f);

    for (int gz = 0; gz < grid; ++gz) {
        for (int gx = 0; gx < grid; ++gx) {
            const int sx = std::min(chunk.vertexOriginX + gx * stride, heightmap.Width() - 1);
            const int sz = std::min(chunk.vertexOriginZ + gz * stride, heightmap.Height() - 1);
            const float localX = static_cast<float>(sx) * dx;
            const float localZ = static_cast<float>(sz) * dz;
            const float y = SampleHeightMeters(heightmap, info, sx, sz);
            const glm::vec3 pos = info.worldOrigin + glm::vec3(localX, y, localZ);
            outMesh.positions.push_back(pos);
            outMesh.normals.push_back(ComputeNormal(heightmap, info, sx, sz));
            outMesh.uvs.emplace_back(
                static_cast<float>(sx) / static_cast<float>(std::max(1, heightmap.Width() - 1)),
                static_cast<float>(sz) / static_cast<float>(std::max(1, heightmap.Height() - 1)));
            bmin = glm::min(bmin, pos);
            bmax = glm::max(bmax, pos);
        }
    }

    for (int gz = 0; gz < grid - 1; ++gz) {
        for (int gx = 0; gx < grid - 1; ++gx) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(gz * grid + gx);
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = i0 + static_cast<std::uint32_t>(grid);
            const std::uint32_t i3 = i2 + 1;
            outMesh.indices.push_back(i0);
            outMesh.indices.push_back(i2);
            outMesh.indices.push_back(i1);
            outMesh.indices.push_back(i1);
            outMesh.indices.push_back(i2);
            outMesh.indices.push_back(i3);
        }
    }

    // Bounds live on the chunk; cast away const via duplicate path — callers update after build.
    // Written through a mutable pattern in Tick: bounds assigned after BuildChunkMesh.
    (void)bmin;
    (void)bmax;
    return true;
}

} // namespace we::runtime::terrain
