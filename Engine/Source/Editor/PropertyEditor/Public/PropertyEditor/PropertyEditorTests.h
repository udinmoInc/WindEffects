// ==============================================================================
// WindEffects — PropertyEditor — PropertyEditorTests
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::property {

struct PROPERTYEDITOR_API PropertyEditorTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct PROPERTYEDITOR_API PropertyEditorTestReport {
    std::vector<PropertyEditorTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] PROPERTYEDITOR_API PropertyEditorTestReport RunPropertyEditorTests();

} // namespace we::editor::property
