// ==============================================================================
// WindEffects — WorldOutliner — WorldOutlinerTests
// Public API surface for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WorldOutliner/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::outliner {

struct WORLDOUTLINER_API WorldOutlinerTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct WORLDOUTLINER_API WorldOutlinerTestReport {
    std::vector<WorldOutlinerTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] WORLDOUTLINER_API WorldOutlinerTestReport RunWorldOutlinerRuntimeTests();

} // namespace we::editor::outliner
