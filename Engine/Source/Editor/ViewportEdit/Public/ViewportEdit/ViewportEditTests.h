// ==============================================================================
// WindEffects — ViewportEdit — ViewportEditTests
// Public API surface for the ViewportEdit module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ViewportEdit/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::viewportedit {

struct VIEWPORTEDIT_API ViewportEditTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct VIEWPORTEDIT_API ViewportEditTestReport {
    std::vector<ViewportEditTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] VIEWPORTEDIT_API ViewportEditTestReport RunViewportEditRuntimeTests();

} // namespace we::editor::viewportedit
