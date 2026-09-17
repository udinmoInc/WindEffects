// ==============================================================================
// WindEffects — ScalabilityHardening — Main
// Runs Scalability runtime tests (profile resolve, capability fallback, switching).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityTests.h"

#include <cstdlib>
#include <iostream>

int main() {
    using we::runtime::renderer::RunScalabilityRuntimeTests;

    std::cout << "Running Scalability runtime tests...\n";
    const auto report = RunScalabilityRuntimeTests();
    for (const auto& testCase : report.cases) {
        std::cout << (testCase.passed ? "[PASS] " : "[FAIL] ")
            << testCase.name;
        if (!testCase.message.empty()) {
            std::cout << " — " << testCase.message;
        }
        std::cout << '\n';
    }
    std::cout << report.summary << '\n';
    return report.success ? EXIT_SUCCESS : EXIT_FAILURE;
}
