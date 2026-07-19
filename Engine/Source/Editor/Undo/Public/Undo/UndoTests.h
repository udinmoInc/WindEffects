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
