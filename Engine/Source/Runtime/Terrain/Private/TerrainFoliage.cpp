#include "Terrain/TerrainFoliage.h"
#include "Terrain/TerrainMaterialSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::terrain {

namespace {

float Hash01(std::uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}

} // namespace

void TerrainFoliageSystem::Clear() {
    m_Instances.clear();
}

int TerrainFoliageSystem::Spawn(const TerrainCollision& collision, const TerrainCreateInfo& info,
    const TerrainMaterialSystem* materials, const FoliageSpawnParams& params,
    const we::math::Vec2& regionMinLocal, const we::math::Vec2& regionMaxLocal) {
    const float minX = std::min(regionMinLocal.x, regionMaxLocal.x);
    const float maxX = std::max(regionMinLocal.x, regionMaxLocal.x);
    const float minZ = std::min(regionMinLocal.y, regionMaxLocal.y);
    const float maxZ = std::max(regionMinLocal.y, regionMaxLocal.y);
    const float area = std::max(0.0f, maxX - minX) * std::max(0.0f, maxZ - minZ);
    const int count = static_cast<int>(std::lround(area * std::max(0.0f, params.densityPerSquareMeter)));
    if (count <= 0) {
        return 0;
    }

    std::uint32_t seed = params.seed ? params.seed : 1u;
    int spawned = 0;

    for (int i = 0; i < count; ++i) {
        seed = seed * 1664525u + 1013904223u;
        const float u = Hash01(seed);
        seed = seed * 1664525u + 1013904223u;
        const float v = Hash01(seed);

        const float localX = minX + u * (maxX - minX);
        const float localZ = minZ + v * (maxZ - minZ);
        const float worldX = info.worldOrigin.x + localX;
        const float worldZ = info.worldOrigin.z + localZ;

        float height = 0.0f;
        we::math::Vec3 normal(0.0f, 1.0f, 0.0f);
        if (!collision.SampleHeightNormal(worldX, worldZ, height, normal)) {
            continue;
        }
        if (height < params.minHeight || height > params.maxHeight) {
            continue;
        }

        const float slopeDeg = std::acos(std::clamp(normal.y, -1.0f, 1.0f)) * 57.2957795f;
        if (slopeDeg < params.minSlopeDegrees || slopeDeg > params.maxSlopeDegrees) {
            continue;
        }

        if (materials && materials->Width() > 0 && materials->Height() > 0) {
            const int sx = std::clamp(static_cast<int>(std::lround(
                (localX / info.worldSizeX) * static_cast<float>(materials->Width() - 1))),
                0, materials->Width() - 1);
            const int sz = std::clamp(static_cast<int>(std::lround(
                (localZ / info.worldSizeY) * static_cast<float>(materials->Height() - 1))),
                0, materials->Height() - 1);
            const std::uint8_t* w = materials->WeightData()
                + (static_cast<std::size_t>(sz) * static_cast<std::size_t>(materials->Width())
                    + static_cast<std::size_t>(sx)) * 4u;
            const int layer = std::clamp(params.layerIndex, 0, 3);
            if (w[layer] < 64) {
                continue;
            }
        }

        seed = seed * 1664525u + 1013904223u;
        const float scaleT = Hash01(seed);
        seed = seed * 1664525u + 1013904223u;
        const float yawT = Hash01(seed);

        FoliageInstance inst{};
        inst.position = we::math::Vec3(worldX, height, worldZ);
        inst.normal = normal;
        inst.yawDegrees = yawT * 360.0f;
        inst.scale = params.minScale + scaleT * (params.maxScale - params.minScale);
        inst.layerIndex = params.layerIndex;
        m_Instances.push_back(inst);
        ++spawned;
    }
    return spawned;
}

} // namespace we::runtime::terrain
