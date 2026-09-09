// ==============================================================================
// WindEffects — TerrainEditor — TerrainEditorTests
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
