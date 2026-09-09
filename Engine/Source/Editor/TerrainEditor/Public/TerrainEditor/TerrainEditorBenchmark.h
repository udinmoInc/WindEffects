// ==============================================================================
// WindEffects — TerrainEditor — TerrainEditorBenchmark
// Public API surface for the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "TerrainEditor/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::terrain {

struct TERRAINEDITOR_API TerrainEditorBenchmarkReport {
    std::uint32_t resolution = 0;
    std::uint32_t brushIterations = 0;
    double createMs = 0.0;
    double brushMs = 0.0;
    double proceduralMs = 0.0;
    std::string summary;
};

[[nodiscard]] TERRAINEDITOR_API TerrainEditorBenchmarkReport RunTerrainEditorBenchmarks(
    std::uint32_t resolution = 505,
    std::uint32_t brushIterations = 64);

} // namespace we::editor::terrain
