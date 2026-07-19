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
