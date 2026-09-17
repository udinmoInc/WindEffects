// ==============================================================================
// WindEffects — Renderer — ScalabilityTests
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"

#include <cstdint>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

struct RENDERER_API ScalabilityTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct RENDERER_API ScalabilityTestReport {
    std::vector<ScalabilityTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] RENDERER_API ScalabilityTestReport RunScalabilityRuntimeTests();

} // namespace we::runtime::renderer

#pragma warning(pop)
