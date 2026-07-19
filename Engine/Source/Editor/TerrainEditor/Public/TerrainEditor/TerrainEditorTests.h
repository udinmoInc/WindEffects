#pragma once

#include "TerrainEditor/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::terrain {

struct TERRAINEDITOR_API TerrainEditorTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct TERRAINEDITOR_API TerrainEditorTestReport {
    bool success = false;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    std::string summary;
    std::vector<TerrainEditorTestCaseResult> cases;
};

[[nodiscard]] TERRAINEDITOR_API TerrainEditorTestReport RunTerrainEditorTests();

} // namespace we::editor::terrain
