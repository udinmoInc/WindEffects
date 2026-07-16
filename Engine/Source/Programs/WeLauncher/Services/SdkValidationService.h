#pragma once

#include <string>
#include <vector>

namespace we::programs::welauncher {

enum class SdkCheckStatus {
    Pass,
    Warn,
    Fail,
};

struct SdkCheckResult {
    std::string name;
    SdkCheckStatus status = SdkCheckStatus::Pass;
    std::string detail;
};

class SdkValidationService {
public:
    [[nodiscard]] std::vector<SdkCheckResult> RunChecks() const;
};

} // namespace we::programs::welauncher
