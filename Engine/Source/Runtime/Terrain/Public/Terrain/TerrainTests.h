// ==============================================================================
// WindEffects — Terrain — TerrainTests
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
