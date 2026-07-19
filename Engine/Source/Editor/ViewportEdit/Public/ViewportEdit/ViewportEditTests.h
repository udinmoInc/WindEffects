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
