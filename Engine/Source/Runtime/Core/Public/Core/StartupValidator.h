// ==============================================================================
// WindEffects — Core — StartupValidator
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"
#include <functional>
#include <string>
#include <vector>

namespace we::runtime::core {

struct StartupCheckResult {
    std::string name;
    bool success = false;
    std::string detail;
};

class StartupValidator {
public:
    CORE_API static StartupValidator& Get();

    CORE_API void RegisterCheck(const std::string& name, std::function<bool(std::string& detail)> check);
    CORE_API std::vector<StartupCheckResult> RunAll();
    CORE_API bool AllPassed() const { return m_AllPassed; }

private:
    std::vector<std::pair<std::string, std::function<bool(std::string&)>>> m_Checks;
    bool m_AllPassed = true;
};

} // namespace we::runtime::core
