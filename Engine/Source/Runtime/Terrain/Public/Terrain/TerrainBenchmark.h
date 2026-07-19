#pragma once

#include "Terrain/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::terrain {

struct TERRAIN_API TerrainBenchmarkResult {
    std::string name;
    std::uint64_t iterations = 0;
    std::uint64_t totalMicros = 0;
    double microsPerOp = 0.0;
};

struct TERRAIN_API TerrainBenchmarkReport {
    std::vector<TerrainBenchmarkResult> results;
    std::string summary;
};

[[nodiscard]] TERRAIN_API TerrainBenchmarkReport RunTerrainRuntimeBenchmarks();

} // namespace we::runtime::terrain
