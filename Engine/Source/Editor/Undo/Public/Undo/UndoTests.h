// ==============================================================================
// WindEffects — Undo — UndoTests
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Undo/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::undo {

struct UNDO_API UndoTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct UNDO_API UndoTestReport {
    std::vector<UndoTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] UNDO_API UndoTestReport RunUndoRuntimeTests();

} // namespace we::editor::undo
