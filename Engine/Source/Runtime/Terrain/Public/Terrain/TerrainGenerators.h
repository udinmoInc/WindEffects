#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/ITerrain.h"
#include "Terrain/TerrainHeightmap.h"

#include <memory>
#include <string_view>

namespace we::runtime::terrain {

class ITerrainGeneratorRegistry;

/// Builtin + plugin generator helpers. Heightmap files are never involved.
class TERRAIN_API TerrainGenerators {
public:
    static void RegisterBuiltins(ITerrainGeneratorRegistry& registry);

    [[nodiscard]] static std::shared_ptr<ITerrainGenerator> MakeBuiltin(TerrainGeneratorId id);

    /// Fill heightfield from creation method / generator params (deterministic).
    static bool Generate(
        TerrainHeightmap& heightfield,
        const TerrainCreateInfo& info,
        const TerrainGeneratorParams& params,
        const ITerrainGeneratorRegistry* registry = nullptr);
};

/// Backward-compatible procedural helper (maps to FBM / Ridged generators).
class TERRAIN_API TerrainProcedural {
public:
    static void Generate(TerrainHeightmap& outMap, const ProceduralHeightmapParams& params);
    static void ApplyRidge(
        TerrainHeightmap& map,
        const ProceduralHeightmapParams& params,
        float ridgeStrength = 0.4f);
};

} // namespace we::runtime::terrain
