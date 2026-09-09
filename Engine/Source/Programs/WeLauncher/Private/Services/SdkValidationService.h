// ==============================================================================
// WindEffects — WeLauncher — SdkValidationService
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
