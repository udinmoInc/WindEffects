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
