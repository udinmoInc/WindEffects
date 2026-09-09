// ==============================================================================
// WindEffects — Core — StartupValidator
// Internal implementation for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/StartupValidator.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

namespace we::runtime::core {

StartupValidator& StartupValidator::Get() {
    static StartupValidator instance;
    return instance;
}

void StartupValidator::RegisterCheck(const std::string& name, std::function<bool(std::string& detail)> check) {
    m_Checks.emplace_back(name, std::move(check));
}

std::vector<StartupCheckResult> StartupValidator::RunAll() {
    m_AllPassed = true;
    std::vector<StartupCheckResult> results;
    results.reserve(m_Checks.size());

    WE_LOG_INFO(LogCategory::Startup.data(), "Running startup validation (" + std::to_string(m_Checks.size()) + " checks)...");

    for (const auto& [name, check] : m_Checks) {
        StartupCheckResult result{};
        result.name = name;
        result.success = check(result.detail);
        results.push_back(result);

        if (result.success) {
            WE_LOG_INFO(LogCategory::Startup.data(), "[PASS] " + name + (result.detail.empty() ? "" : " - " + result.detail));
        } else {
            m_AllPassed = false;
            WE_LOG_ERROR(LogCategory::Startup.data(), "[FAIL] " + name + " - " + result.detail);
        }
    }

    if (m_AllPassed) {
        WE_LOG_INFO(LogCategory::Startup.data(), "Startup validation passed.");
    } else {
        WE_LOG_CRITICAL(LogCategory::Startup.data(), "Startup validation failed. See log for details.");
    }

    return results;
}

} // namespace we::runtime::core
