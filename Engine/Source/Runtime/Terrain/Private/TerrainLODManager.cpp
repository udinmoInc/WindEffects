#include "Terrain/TerrainLODManager.h"
#include "Terrain/TerrainHeightmap.h"
#include "Terrain/TerrainDiagnostics.h"

#include <algorithm>
#include <cmath>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::terrain {

namespace {

float SampleHeightMeters(const TerrainHeightmap& map, const TerrainCreateInfo& info, int x, int z) {
    const float n = static_cast<float>(map.Get(x, z)) / 65535.0f;
    return info.heightOffset + n * info.heightScale * info.worldScale.y;
}

we::math::Vec3 ComputeNormal(const TerrainHeightmap& map, const TerrainCreateInfo& info, int x, int z) {
    const int x0 = std::max(0, x - 1);
    const int x1 = std::min(map.Width() - 1, x + 1);
    const int z0 = std::max(0, z - 1);
    const int z1 = std::min(map.Height() - 1, z + 1);
    const float hL = SampleHeightMeters(map, info, x0, z);
    const float hR = SampleHeightMeters(map, info, x1, z);
    const float hD = SampleHeightMeters(map, info, x, z0);
    const float hU = SampleHeightMeters(map, info, x, z1);
    const float dx = (info.worldSizeX * info.worldScale.x)
        / static_cast<float>(std::max(1, map.Width() - 1));
    const float dz = (info.worldSizeY * info.worldScale.z)
        / static_cast<float>(std::max(1, map.Height() - 1));
    const float spanX = dx * static_cast<float>(std::max(1, x1 - x0));
    const float spanZ = dz * static_cast<float>(std::max(1, z1 - z0));
    we::math::Vec3 n(-(hR - hL) / spanX, 1.0f, -(hU - hD) / spanZ);
    const float len = we::math::Length(n);
    return (len > 1e-6f) ? (n / len) : we::math::Vec3(0.0f, 1.0f, 0.0f);
}

float ChunkWorldExtent(const TerrainChunk& chunk, const TerrainCreateInfo& info, int heightmapWidth) {
    const float dx = (info.worldSizeX * info.worldScale.x)
        / static_cast<float>(std::max(1, heightmapWidth - 1));
    return dx * static_cast<float>(std::max(1, chunk.quads));
}

} // namespace

int TerrainLODManager::SelectLod(const TerrainChunk& chunk, const we::math::Vec3& cameraWorldPos,
    float worldSizeX, float worldSizeY, int heightmapWidth) const {
    (void)worldSizeY;
    const we::math::Vec3 center = (chunk.bounds.min + chunk.bounds.max) * 0.5f;
    const float dist = we::math::Length(cameraWorldPos - center);
    float chunkWorld = 0.0f;
    if (chunk.bounds.min.x <= chunk.bounds.max.x) {
        chunkWorld = std::max(
            chunk.bounds.max.x - chunk.bounds.min.x,
            chunk.bounds.max.z - chunk.bounds.min.z);
    }
    if (chunkWorld < 1.0f) {
        chunkWorld = worldSizeX / static_cast<float>(std::max(1, heightmapWidth > 1 ? 8 : 1));
    }
    // lodBias > 1 coarsens sooner (AAA editor FPS bias).
    const float relative = (dist / std::max(1.0f, chunkWorld)) * std::max(0.25f, m_LodBias);
    int lod = 0;
    // Aggressive thresholds in chunk extents — only near camera stays LOD0.
    if (relative > 1.15f) lod = 1;
    if (relative > 2.25f) lod = 2;
    if (relative > 4.50f) lod = 3;
    if (relative > 8.00f) lod = 4;
    return std::min(lod, m_MaxLod);
}

void TerrainLODManager::UpdateChunkLods(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
    const TerrainCreateInfo& info, int heightmapWidth) {
    m_LodBias = std::max(0.25f, info.lod.lodBias);
    m_MaxLod = std::clamp(info.lod.maxLod, 0, kMaxLodLevels - 1);

    std::uint64_t changed = 0;
    const float worldX = info.worldSizeX * info.worldScale.x;
    const float worldZ = info.worldSizeY * info.worldScale.z;
    for (TerrainChunk& chunk : chunks.Chunks()) {
        if (!chunk.visible) {
            continue;
        }
        if (!(chunk.bounds.min.x <= chunk.bounds.max.x)) {
            continue;
        }
        const int desired = SelectLod(chunk, cameraWorldPos, worldX, worldZ, heightmapWidth);
        if (desired == chunk.lod) {
            continue;
        }
        // Allow single-step refine toward camera; require 2 steps to coarsen (avoids thrash).
        const bool refine = desired < chunk.lod;
        const bool coarsen = desired > chunk.lod + 1;
        if (!refine && !coarsen) {
            continue;
        }
        if (coarsen) {
            const float chunkWorld = ChunkWorldExtent(chunk, info, heightmapWidth);
            const we::math::Vec3 center = (chunk.bounds.min + chunk.bounds.max) * 0.5f;
            const float dist = we::math::Length(cameraWorldPos - center);
            const float relative = (dist / std::max(1.0f, chunkWorld)) * std::max(0.25f, m_LodBias);
            const float thresholds[] = {1.15f, 2.25f, 4.50f, 8.00f};
            const int band = std::clamp(desired - 1, 0, 3);
            if (std::abs(relative - thresholds[band]) < 0.35f) {
                continue;
            }
        }
        chunk.lod = desired;
        chunk.meshDirty = true;
        ++changed;
    }
    if (changed > 0) {
        TerrainDiagnostics::Get().OnLodUpdated(changed);
    }
}

void TerrainLODManager::SeedChunkLods(TerrainChunkManager& chunks, const we::math::Vec3& cameraWorldPos,
    const TerrainCreateInfo& info, int heightmapWidth) {
    m_LodBias = std::max(0.25f, info.lod.lodBias);
    m_MaxLod = std::clamp(info.lod.maxLod, 0, kMaxLodLevels - 1);
    const float worldX = info.worldSizeX * info.worldScale.x;
    const float worldZ = info.worldSizeY * info.worldScale.z;
    for (TerrainChunk& chunk : chunks.Chunks()) {
        if (!(chunk.bounds.min.x <= chunk.bounds.max.x)) {
            continue;
        }
        const int lod = SelectLod(chunk, cameraWorldPos, worldX, worldZ, heightmapWidth);
        if (chunk.lod != lod) {
            chunk.lod = lod;
            chunk.meshDirty = true;
            chunk.gpuDirty = true;
        }
    }
}

bool TerrainLODManager::BuildChunkMesh(const TerrainHeightmap& heightmap, const TerrainCreateInfo& info,
    const TerrainChunk& chunk, int lod, TerrainMeshCPU& outMesh) {
    if (heightmap.Empty()) {
        return false;
    }

    // 127-quad sections are odd-width — do NOT require quads % stride == 0.
    // Step across the chunk and always pin the last ring to the far edge.
    const int stride = 1 << std::clamp(lod, 0, kMaxLodLevels - 1);
    const int quads = std::max(1, chunk.quads);
    const int gridQuads = (quads + stride - 1) / stride;
    const int grid = gridQuads + 1;
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

    const float dx = (info.worldSizeX * info.worldScale.x)
        / static_cast<float>(std::max(1, heightmap.Width() - 1));
    const float dz = (info.worldSizeY * info.worldScale.z)
        / static_cast<float>(std::max(1, heightmap.Height() - 1));

    const int endX = std::min(chunk.vertexOriginX + quads, heightmap.Width() - 1);
    const int endZ = std::min(chunk.vertexOriginZ + quads, heightmap.Height() - 1);
    const bool cheapNormals = lod >= 2;

    for (int gz = 0; gz < grid; ++gz) {
        for (int gx = 0; gx < grid; ++gx) {
            const int sx = (gx == grid - 1)
                ? endX
                : std::min(chunk.vertexOriginX + gx * stride, endX);
            const int sz = (gz == grid - 1)
                ? endZ
                : std::min(chunk.vertexOriginZ + gz * stride, endZ);
            const float localX = static_cast<float>(sx) * dx;
            const float localZ = static_cast<float>(sz) * dz;
            const float y = SampleHeightMeters(heightmap, info, sx, sz);
            const we::math::Vec3 pos = info.worldOrigin + we::math::Vec3(localX, y, localZ);
            outMesh.positions.push_back(pos);
            if (cheapNormals) {
                outMesh.normals.emplace_back(0.0f, 1.0f, 0.0f);
            } else {
                outMesh.normals.push_back(ComputeNormal(heightmap, info, sx, sz));
            }
            outMesh.uvs.emplace_back(
                static_cast<float>(sx) / static_cast<float>(std::max(1, heightmap.Width() - 1)),
                static_cast<float>(sz) / static_cast<float>(std::max(1, heightmap.Height() - 1)));
        }
    }

    for (int gz = 0; gz < grid - 1; ++gz) {
        for (int gx = 0; gx < grid - 1; ++gx) {
            const std::uint32_t i0 = static_cast<std::uint32_t>(gz * grid + gx);
            const std::uint32_t i1 = i0 + 1;
            const std::uint32_t i2 = i0 + static_cast<std::uint32_t>(grid);
            const std::uint32_t i3 = i2 + 1;
            outMesh.indices.push_back(i0);
            outMesh.indices.push_back(i1);
            outMesh.indices.push_back(i2);
            outMesh.indices.push_back(i1);
            outMesh.indices.push_back(i3);
            outMesh.indices.push_back(i2);
        }
    }

    return true;
}

} // namespace we::runtime::terrain
