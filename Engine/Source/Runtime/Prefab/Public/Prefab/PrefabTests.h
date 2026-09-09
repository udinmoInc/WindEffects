// ==============================================================================
// WindEffects — Prefab — PrefabTests
// Public API surface for the Prefab module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Prefab/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::prefab {

struct PREFAB_API PrefabTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct PREFAB_API PrefabTestReport {
    std::vector<PrefabTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] PREFAB_API PrefabTestReport RunPrefabRuntimeTests();

} // namespace we::runtime::prefab
