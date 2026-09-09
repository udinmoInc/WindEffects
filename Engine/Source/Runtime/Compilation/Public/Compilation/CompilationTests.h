// ==============================================================================
// WindEffects — Compilation — CompilationTests
// Public API surface for the Compilation module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Compilation/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::compilation {

struct COMPILATION_API CompilationTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct COMPILATION_API CompilationTestReport {
    std::vector<CompilationTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] COMPILATION_API CompilationTestReport RunCompilationRuntimeTests();

} // namespace we::runtime::compilation
