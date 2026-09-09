// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserTests
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::contentbrowser {

struct CONTENTBROWSER_API ContentBrowserTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct CONTENTBROWSER_API ContentBrowserTestReport {
    std::vector<ContentBrowserTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] CONTENTBROWSER_API ContentBrowserTestReport RunContentBrowserRuntimeTests();

} // namespace we::editor::contentbrowser
