// ==============================================================================
// WindEffects — Terrain — TerrainBenchmark
// Public API surface for the Terrain module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
