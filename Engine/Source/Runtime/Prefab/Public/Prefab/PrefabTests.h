#pragma once

#include "Prefab/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::prefab {

struct PREFAB_API PrefabTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct PREFAB_API PrefabTestReport {
    std::vector<PrefabTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] PREFAB_API PrefabTestReport RunPrefabRuntimeTests();

} // namespace we::runtime::prefab
