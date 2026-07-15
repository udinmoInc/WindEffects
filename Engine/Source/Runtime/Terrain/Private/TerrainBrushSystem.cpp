#include "Terrain/TerrainBrushSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace we::runtime::terrain {

namespace {

std::uint16_t ClampU16(int v) {
    return static_cast<std::uint16_t>(std::clamp(v, 0, 65535));
}

float Hash01(int x, int z, std::uint32_t seed) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u
        + static_cast<std::uint32_t>(z) * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu);
}

} // namespace

float TerrainBrushSystem::FalloffWeight(float dist, float radius) const {
    if (radius <= 1e-6f || dist >= radius) {
        return 0.0f;
    }
    const float t = 1.0f - dist / radius;
    const float soft = std::clamp(m_Settings.falloff, 0.0f, 1.0f);
    // Hard center, soft edge controlled by falloff.
    return std::pow(t, 1.0f + soft * 2.0f);
}

bool TerrainBrushSystem::Apply(TerrainHeightmap& heightmap, float centerX, float centerZ) {
    switch (m_Settings.op) {
    case TerrainBrushOp::Raise: return RaiseLower(heightmap, centerX, centerZ, true);
    case TerrainBrushOp::Lower: return RaiseLower(heightmap, centerX, centerZ, false);
    case TerrainBrushOp::Smooth: return Smooth(heightmap, centerX, centerZ);
    case TerrainBrushOp::Flatten: return Flatten(heightmap, centerX, centerZ);
    case TerrainBrushOp::Noise: return Noise(heightmap, centerX, centerZ);
    case TerrainBrushOp::HydraulicErosion: return HydraulicErosion(heightmap, centerX, centerZ);
    case TerrainBrushOp::ThermalErosion: return ThermalErosion(heightmap, centerX, centerZ);
    }
    return false;
}

bool TerrainBrushSystem::RaiseLower(TerrainHeightmap& map, float cx, float cz, bool raise) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    const int delta = static_cast<int>(std::lround(m_Settings.strength * 2000.0f)) * (raise ? 1 : -1);
    bool changed = false;

    for (int z = icz - r; z <= icz + r; ++z) {
        for (int x = icx - r; x <= icx + r; ++x) {
            if (x < 0 || z < 0 || x >= map.Width() || z >= map.Height()) {
                continue;
            }
            const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                + (z - icz) * (z - icz)));
            const float w = FalloffWeight(dist, m_Settings.radius);
            if (w <= 0.0f) {
                continue;
            }
            const int value = static_cast<int>(map.Get(x, z))
                + static_cast<int>(std::lround(static_cast<float>(delta) * w));
            map.Set(x, z, ClampU16(value));
            changed = true;
        }
    }
    return changed;
}

bool TerrainBrushSystem::Smooth(TerrainHeightmap& map, float cx, float cz) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    std::vector<std::pair<int, int>> coords;
    std::vector<std::uint16_t> next;

    for (int z = icz - r; z <= icz + r; ++z) {
        for (int x = icx - r; x <= icx + r; ++x) {
            if (x < 0 || z < 0 || x >= map.Width() || z >= map.Height()) {
                continue;
            }
            const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                + (z - icz) * (z - icz)));
            if (FalloffWeight(dist, m_Settings.radius) <= 0.0f) {
                continue;
            }
            float sum = 0.0f;
            int count = 0;
            for (int oz = -1; oz <= 1; ++oz) {
                for (int ox = -1; ox <= 1; ++ox) {
                    const int sx = x + ox;
                    const int sz = z + oz;
                    if (sx < 0 || sz < 0 || sx >= map.Width() || sz >= map.Height()) {
                        continue;
                    }
                    sum += static_cast<float>(map.Get(sx, sz));
                    ++count;
                }
            }
            if (count == 0) {
                continue;
            }
            const float avg = sum / static_cast<float>(count);
            const float w = FalloffWeight(dist, m_Settings.radius) * m_Settings.strength;
            const float blended = static_cast<float>(map.Get(x, z)) * (1.0f - w) + avg * w;
            coords.emplace_back(x, z);
            next.push_back(ClampU16(static_cast<int>(std::lround(blended))));
        }
    }

    for (std::size_t i = 0; i < coords.size(); ++i) {
        map.Set(coords[i].first, coords[i].second, next[i]);
    }
    return !coords.empty();
}

bool TerrainBrushSystem::Flatten(TerrainHeightmap& map, float cx, float cz) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    const std::uint16_t target = ClampU16(static_cast<int>(
        std::lround(std::clamp(m_Settings.targetHeight, 0.0f, 1.0f) * 65535.0f)));
    bool changed = false;

    for (int z = icz - r; z <= icz + r; ++z) {
        for (int x = icx - r; x <= icx + r; ++x) {
            if (x < 0 || z < 0 || x >= map.Width() || z >= map.Height()) {
                continue;
            }
            const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                + (z - icz) * (z - icz)));
            const float w = FalloffWeight(dist, m_Settings.radius) * m_Settings.strength;
            if (w <= 0.0f) {
                continue;
            }
            const float cur = static_cast<float>(map.Get(x, z));
            const float blended = cur * (1.0f - w) + static_cast<float>(target) * w;
            map.Set(x, z, ClampU16(static_cast<int>(std::lround(blended))));
            changed = true;
        }
    }
    return changed;
}

bool TerrainBrushSystem::Noise(TerrainHeightmap& map, float cx, float cz) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    const float amp = m_Settings.strength * m_Settings.noiseScale * 4000.0f;
    bool changed = false;

    for (int z = icz - r; z <= icz + r; ++z) {
        for (int x = icx - r; x <= icx + r; ++x) {
            if (x < 0 || z < 0 || x >= map.Width() || z >= map.Height()) {
                continue;
            }
            const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                + (z - icz) * (z - icz)));
            const float w = FalloffWeight(dist, m_Settings.radius);
            if (w <= 0.0f) {
                continue;
            }
            const float n = Hash01(x, z, 1337u) * 2.0f - 1.0f;
            const int value = static_cast<int>(map.Get(x, z))
                + static_cast<int>(std::lround(n * amp * w));
            map.Set(x, z, ClampU16(value));
            changed = true;
        }
    }
    return changed;
}

bool TerrainBrushSystem::HydraulicErosion(TerrainHeightmap& map, float cx, float cz) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    const int iterations = std::max(1, m_Settings.erosionIterations);
    bool changed = false;

    for (int iter = 0; iter < iterations; ++iter) {
        for (int z = icz - r; z <= icz + r; ++z) {
            for (int x = icx - r; x <= icx + r; ++x) {
                if (x <= 0 || z <= 0 || x >= map.Width() - 1 || z >= map.Height() - 1) {
                    continue;
                }
                const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                    + (z - icz) * (z - icz)));
                const float w = FalloffWeight(dist, m_Settings.radius) * m_Settings.strength;
                if (w <= 0.0f) {
                    continue;
                }

                const int h = static_cast<int>(map.Get(x, z));
                int lowest = h;
                int lx = x;
                int lz = z;
                for (int oz = -1; oz <= 1; ++oz) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if (ox == 0 && oz == 0) {
                            continue;
                        }
                        const int nh = static_cast<int>(map.Get(x + ox, z + oz));
                        if (nh < lowest) {
                            lowest = nh;
                            lx = x + ox;
                            lz = z + oz;
                        }
                    }
                }
                if (lowest >= h) {
                    continue;
                }
                const int sediment = static_cast<int>(std::lround(static_cast<float>(h - lowest)
                    * 0.15f * w));
                if (sediment <= 0) {
                    continue;
                }
                map.Set(x, z, ClampU16(h - sediment));
                map.Set(lx, lz, ClampU16(lowest + sediment));
                changed = true;
            }
        }
    }
    return changed;
}

bool TerrainBrushSystem::ThermalErosion(TerrainHeightmap& map, float cx, float cz) {
    if (map.Empty()) {
        return false;
    }
    const int icx = static_cast<int>(std::lround(cx));
    const int icz = static_cast<int>(std::lround(cz));
    const int r = static_cast<int>(std::ceil(m_Settings.radius));
    const int iterations = std::max(1, m_Settings.erosionIterations);
    const int talus = static_cast<int>(800.0f * (1.0f - std::clamp(m_Settings.strength, 0.0f, 1.0f)));
    bool changed = false;

    for (int iter = 0; iter < iterations; ++iter) {
        for (int z = icz - r; z <= icz + r; ++z) {
            for (int x = icx - r; x <= icx + r; ++x) {
                if (x <= 0 || z <= 0 || x >= map.Width() - 1 || z >= map.Height() - 1) {
                    continue;
                }
                const float dist = std::sqrt(static_cast<float>((x - icx) * (x - icx)
                    + (z - icz) * (z - icz)));
                if (FalloffWeight(dist, m_Settings.radius) <= 0.0f) {
                    continue;
                }

                const int h = static_cast<int>(map.Get(x, z));
                for (int oz = -1; oz <= 1; ++oz) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        if (ox == 0 && oz == 0) {
                            continue;
                        }
                        const int nh = static_cast<int>(map.Get(x + ox, z + oz));
                        const int diff = h - nh;
                        if (diff <= talus) {
                            continue;
                        }
                        const int move = (diff - talus) / 2;
                        map.Set(x, z, ClampU16(h - move));
                        map.Set(x + ox, z + oz, ClampU16(nh + move));
                        changed = true;
                    }
                }
            }
        }
    }
    return changed;
}

} // namespace we::runtime::terrain
