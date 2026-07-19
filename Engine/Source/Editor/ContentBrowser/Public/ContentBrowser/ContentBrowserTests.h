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
