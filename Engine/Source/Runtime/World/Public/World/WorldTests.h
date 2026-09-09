// ==============================================================================
// WindEffects — World — WorldTests
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"
#include "World/WorldDiagnostics.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::world {

struct WORLD_API WorldTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct WORLD_API WorldTestReport {
    std::vector<WorldTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] WORLD_API WorldTestReport RunWorldRuntimeTests();

} // namespace we::runtime::world
