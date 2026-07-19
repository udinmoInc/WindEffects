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
