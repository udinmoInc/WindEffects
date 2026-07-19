#pragma once

#include "World/Export.h"
#include "World/WorldDiagnostics.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::world {

struct WORLD_API WorldTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct WORLD_API WorldTestReport {
    std::vector<WorldTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] WORLD_API WorldTestReport RunWorldRuntimeTests();

} // namespace we::runtime::world
