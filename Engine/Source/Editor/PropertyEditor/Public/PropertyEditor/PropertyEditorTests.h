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
