#pragma once

#include "Terrain/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::terrain {

struct TERRAIN_API TerrainTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct TERRAIN_API TerrainTestReport {
    std::vector<TerrainTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] TERRAIN_API TerrainTestReport RunTerrainRuntimeTests();

} // namespace we::runtime::terrain
