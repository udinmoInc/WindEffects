#include "Terrain/TerrainGenerators.h"
#include "Terrain/ITerrainRuntime.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::terrain {
namespace {

float Hash01(std::uint32_t x, std::uint32_t y, std::uint32_t seed) {
    std::uint32_t h = x * 374761393u + y * 668265263u + seed * 362437u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return static_cast<float>(h & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

float Smoothstep(float t) {
    return t * t * (3.f - 2.f * t);
}

float Fade(float t) {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

float Grad(std::uint32_t hash, float x, float y) {
    switch (hash & 3u) {
    case 0: return x + y;
    case 1: return -x + y;
    case 2: return x - y;
    default: return -x - y;
    }
}

float Perlin2D(float x, float y, std::uint32_t seed) {
    const float x0 = std::floor(x);
    const float y0 = std::floor(y);
    const float fx = x - x0;
    const float fy = y - y0;
    const auto ix = static_cast<std::uint32_t>(static_cast<int>(x0));
    const auto iy = static_cast<std::uint32_t>(static_cast<int>(y0));
    const float u = Fade(fx);
    const float v = Fade(fy);
    const float g00 = Grad(static_cast<std::uint32_t>(Hash01(ix, iy, seed) * 4.f), fx, fy);
    const float g10 = Grad(static_cast<std::uint32_t>(Hash01(ix + 1u, iy, seed) * 4.f), fx - 1.f, fy);
    const float g01 = Grad(static_cast<std::uint32_t>(Hash01(ix, iy + 1u, seed) * 4.f), fx, fy - 1.f);
    const float g11 =
        Grad(static_cast<std::uint32_t>(Hash01(ix + 1u, iy + 1u, seed) * 4.f), fx - 1.f, fy - 1.f);
    const float a = g00 + (g10 - g00) * u;
    const float b = g01 + (g11 - g01) * u;
    return (a + (b - a) * v) * 0.5f + 0.5f;
}

float ValueNoise2D(float x, float y, std::uint32_t seed) {
    const float x0 = std::floor(x);
    const float y0 = std::floor(y);
    const float fx = Smoothstep(x - x0);
    const float fy = Smoothstep(y - y0);
    const auto ix = static_cast<std::uint32_t>(static_cast<int>(x0));
    const auto iy = static_cast<std::uint32_t>(static_cast<int>(y0));
    const float v00 = Hash01(ix, iy, seed);
    const float v10 = Hash01(ix + 1u, iy, seed);
    const float v01 = Hash01(ix, iy + 1u, seed);
    const float v11 = Hash01(ix + 1u, iy + 1u, seed);
    const float a = v00 + (v10 - v00) * fx;
    const float b = v01 + (v11 - v01) * fx;
    return a + (b - a) * fy;
}

float Fbm2D(float x, float y, const TerrainGeneratorParams& params, bool perlin) {
    float amp = 1.f;
    float freq = params.frequency;
    float sum = 0.f;
    float norm = 0.f;
    for (int i = 0; i < params.octaves; ++i) {
        const float n = perlin
            ? Perlin2D(x * freq, y * freq, params.seed + static_cast<std::uint32_t>(i) * 1013u)
            : ValueNoise2D(x * freq, y * freq, params.seed + static_cast<std::uint32_t>(i) * 1013u);
        sum += n * amp;
        norm += amp;
        amp *= params.persistence;
        freq *= params.lacunarity;
    }
    return norm > 0.f ? sum / norm : 0.f;
}

float Ridged2D(float x, float y, const TerrainGeneratorParams& params) {
    const float n = Fbm2D(x, y, params, true);
    return 1.f - std::abs(2.f * n - 1.f);
}

float Voronoi2D(float x, float y, std::uint32_t seed) {
    const float xi = std::floor(x);
    const float yi = std::floor(y);
    float minDist = 1e9f;
    for (int oz = -1; oz <= 1; ++oz) {
        for (int ox = -1; ox <= 1; ++ox) {
            const float cx = xi + static_cast<float>(ox);
            const float cy = yi + static_cast<float>(oz);
            const float px = cx + Hash01(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), seed);
            const float py =
                cy + Hash01(static_cast<std::uint32_t>(cx), static_cast<std::uint32_t>(cy), seed + 17u);
            const float dx = px - x;
            const float dy = py - y;
            minDist = std::min(minDist, dx * dx + dy * dy);
        }
    }
    return std::clamp(std::sqrt(minDist), 0.f, 1.f);
}

void FillHeight(
    TerrainHeightmap& map,
    const TerrainGeneratorParams& params,
    const std::function<float(int, int, int, int)>& sampleFn)
{
    if (map.Empty()) {
        return;
    }
    const int w = map.Width();
    const int h = map.Height();
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            float height = sampleFn(x, z, w, h);
            height = std::clamp(height, 0.f, 1.f);
            map.Set(x, z, static_cast<std::uint16_t>(height * 65535.f + 0.5f));
        }
    }
    map.MarkRegionDirty(0, 0, w - 1, h - 1);
    (void)params;
}

class BuiltinGenerator final : public ITerrainGenerator {
public:
    BuiltinGenerator(TerrainGeneratorId id, std::string name, std::string display)
        : m_Id(id)
        , m_Name(std::move(name))
        , m_Display(std::move(display))
    {}

    [[nodiscard]] std::string_view GetId() const noexcept override { return m_Name; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return m_Display; }
    [[nodiscard]] TerrainGeneratorId GetBuiltinId() const noexcept override { return m_Id; }

    [[nodiscard]] bool Generate(
        TerrainHeightmap& heightfield,
        const TerrainCreateInfo& info,
        const TerrainGeneratorParams& params) const override
    {
        switch (m_Id) {
        case TerrainGeneratorId::Flat: {
            const float elev = std::clamp(
                params.baseHeight > 0.f ? params.baseHeight : info.initialElevation,
                0.f,
                1.f);
            const auto fill = static_cast<std::uint16_t>(elev * 65535.f + 0.5f);
            heightfield.Clear(fill);
            return true;
        }
        case TerrainGeneratorId::Empty:
            heightfield.Clear(0);
            return true;
        case TerrainGeneratorId::PerlinNoise:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                const float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), params, true);
                return params.baseHeight + (n - 0.5f) * 2.f * params.amplitude;
            });
            return true;
        case TerrainGeneratorId::Fbm:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                const float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), params, false);
                return params.baseHeight + (n - 0.5f) * 2.f * params.amplitude;
            });
            return true;
        case TerrainGeneratorId::RidgedNoise:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                const float ridge = Ridged2D(static_cast<float>(x), static_cast<float>(z), params);
                return params.baseHeight + (ridge - 0.5f) * 2.f * params.amplitude
                    * params.ridgeStrength;
            });
            return true;
        case TerrainGeneratorId::Voronoi:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                const float n = Voronoi2D(
                    static_cast<float>(x) * params.frequency * 40.f,
                    static_cast<float>(z) * params.frequency * 40.f,
                    params.seed);
                return params.baseHeight + (1.f - n) * params.amplitude;
            });
            return true;
        case TerrainGeneratorId::Island:
            FillHeight(heightfield, params, [&](int x, int z, int w, int h) {
                const float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), params, true);
                const float nx = (static_cast<float>(x) / static_cast<float>(std::max(1, w - 1))) * 2.f
                    - 1.f;
                const float nz = (static_cast<float>(z) / static_cast<float>(std::max(1, h - 1))) * 2.f
                    - 1.f;
                const float d = std::sqrt(nx * nx + nz * nz);
                const float mask = std::clamp(1.f - d / std::max(0.01f, params.islandFalloff), 0.f, 1.f);
                return (params.baseHeight + (n - 0.5f) * 2.f * params.amplitude) * mask;
            });
            return true;
        case TerrainGeneratorId::Mountain:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                const float ridge = Ridged2D(static_cast<float>(x), static_cast<float>(z), params);
                const float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), params, true);
                const float peak = std::pow(ridge, 1.5f) * params.mountainPeak;
                return params.baseHeight * 0.5f + peak * params.amplitude + n * 0.1f;
            });
            return true;
        case TerrainGeneratorId::Plateau:
            FillHeight(heightfield, params, [&](int x, int z, int, int) {
                float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), params, true);
                n = params.baseHeight + (n - 0.5f) * 2.f * params.amplitude;
                if (n > params.plateauLevel) {
                    n = params.plateauLevel + (n - params.plateauLevel) * 0.15f;
                }
                return n;
            });
            return true;
        case TerrainGeneratorId::RandomSeed: {
            TerrainGeneratorParams seeded = params;
            seeded.seed = params.seed == 0
                ? 1u
                : (params.seed * 1664525u + 1013904223u);
            seeded.generator = TerrainGeneratorId::Fbm;
            FillHeight(heightfield, seeded, [&](int x, int z, int, int) {
                const float n = Fbm2D(static_cast<float>(x), static_cast<float>(z), seeded, true);
                return seeded.baseHeight + (n - 0.5f) * 2.f * seeded.amplitude;
            });
            return true;
        }
        case TerrainGeneratorId::Custom:
        default:
            return false;
        }
    }

private:
    TerrainGeneratorId m_Id;
    std::string m_Name;
    std::string m_Display;
};

} // namespace

void TerrainGenerators::RegisterBuiltins(ITerrainGeneratorRegistry& registry) {
    const TerrainGeneratorId ids[] = {
        TerrainGeneratorId::Flat,
        TerrainGeneratorId::Empty,
        TerrainGeneratorId::PerlinNoise,
        TerrainGeneratorId::Fbm,
        TerrainGeneratorId::RidgedNoise,
        TerrainGeneratorId::Voronoi,
        TerrainGeneratorId::Island,
        TerrainGeneratorId::Mountain,
        TerrainGeneratorId::Plateau,
        TerrainGeneratorId::RandomSeed,
    };
    for (TerrainGeneratorId id : ids) {
        (void)registry.Register(MakeBuiltin(id));
    }
}

std::shared_ptr<ITerrainGenerator> TerrainGenerators::MakeBuiltin(TerrainGeneratorId id) {
    switch (id) {
    case TerrainGeneratorId::Flat:
        return std::make_shared<BuiltinGenerator>(id, "Flat", "Flat");
    case TerrainGeneratorId::Empty:
        return std::make_shared<BuiltinGenerator>(id, "Empty", "Empty");
    case TerrainGeneratorId::PerlinNoise:
        return std::make_shared<BuiltinGenerator>(id, "PerlinNoise", "Perlin Noise");
    case TerrainGeneratorId::Fbm:
        return std::make_shared<BuiltinGenerator>(id, "FBM", "FBM");
    case TerrainGeneratorId::RidgedNoise:
        return std::make_shared<BuiltinGenerator>(id, "RidgedNoise", "Ridged Noise");
    case TerrainGeneratorId::Voronoi:
        return std::make_shared<BuiltinGenerator>(id, "Voronoi", "Voronoi");
    case TerrainGeneratorId::Island:
        return std::make_shared<BuiltinGenerator>(id, "Island", "Island");
    case TerrainGeneratorId::Mountain:
        return std::make_shared<BuiltinGenerator>(id, "Mountain", "Mountain");
    case TerrainGeneratorId::Plateau:
        return std::make_shared<BuiltinGenerator>(id, "Plateau", "Plateau");
    case TerrainGeneratorId::RandomSeed:
        return std::make_shared<BuiltinGenerator>(id, "RandomSeed", "Random Seed");
    default:
        return nullptr;
    }
}

bool TerrainGenerators::Generate(
    TerrainHeightmap& heightfield,
    const TerrainCreateInfo& info,
    const TerrainGeneratorParams& params,
    const ITerrainGeneratorRegistry* registry)
{
    if (params.generator == TerrainGeneratorId::Custom && registry
        && !params.pluginGeneratorId.empty())
    {
        if (ITerrainGenerator* plugin = registry->Find(params.pluginGeneratorId)) {
            return plugin->Generate(heightfield, info, params);
        }
        return false;
    }
    auto builtin = MakeBuiltin(params.generator);
    if (!builtin) {
        builtin = MakeBuiltin(TerrainGeneratorId::Flat);
    }
    return builtin && builtin->Generate(heightfield, info, params);
}

void TerrainProcedural::Generate(TerrainHeightmap& outMap, const ProceduralHeightmapParams& params) {
    TerrainGeneratorParams p = params;
    if (p.generator == TerrainGeneratorId::Flat) {
        p.generator = TerrainGeneratorId::Fbm;
    }
    TerrainCreateInfo info{};
    info.resolutionX = outMap.Width();
    info.resolutionY = outMap.Height();
    (void)TerrainGenerators::Generate(outMap, info, p, nullptr);
}

void TerrainProcedural::ApplyRidge(
    TerrainHeightmap& map,
    const ProceduralHeightmapParams& params,
    float ridgeStrength)
{
    if (map.Empty()) {
        return;
    }
    TerrainGeneratorParams p = params;
    p.ridgeStrength = ridgeStrength;
    p.generator = TerrainGeneratorId::RidgedNoise;
    const int w = map.Width();
    const int h = map.Height();
    for (int z = 0; z < h; ++z) {
        for (int x = 0; x < w; ++x) {
            const float ridge = Ridged2D(static_cast<float>(x), static_cast<float>(z), p);
            const float cur = static_cast<float>(map.Get(x, z)) / 65535.f;
            const float mixed = cur * (1.f - ridgeStrength) + ridge * ridgeStrength;
            map.Set(x, z, static_cast<std::uint16_t>(std::clamp(mixed, 0.f, 1.f) * 65535.f + 0.5f));
        }
    }
    map.MarkRegionDirty(0, 0, w - 1, h - 1);
}

} // namespace we::runtime::terrain
