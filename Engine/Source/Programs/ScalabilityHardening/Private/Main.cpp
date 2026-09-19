// ==============================================================================
// WindEffects — ScalabilityHardening — Main
// Runs Scalability runtime tests (profile resolve, capability fallback, switching).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityTests.h"
#include "Lighting/LightingTests.h"

#include <cstdlib>
#include <iostream>

int main(int /*argc*/, char** /*argv*/) {
    using we::runtime::renderer::RunScalabilityRuntimeTests;
    using we::runtime::renderer::RunLightingRuntimeTests;

    bool ok = true;

    std::cout << "Running Scalability runtime tests...\n";
    const auto scalability = RunScalabilityRuntimeTests();
    for (const auto& testCase : scalability.cases) {
        std::cout << (testCase.passed ? "[PASS] " : "[FAIL] ")
            << testCase.name;
        if (!testCase.message.empty()) {
            std::cout << " — " << testCase.message;
        }
        std::cout << '\n';
    }
    std::cout << scalability.summary << '\n';
    ok = ok && scalability.success;

    std::cout << "\nRunning Lighting runtime tests...\n";
    const auto lighting = RunLightingRuntimeTests();
    for (const auto& testCase : lighting.cases) {
        std::cout << (testCase.passed ? "[PASS] " : "[FAIL] ")
            << testCase.name;
        if (!testCase.message.empty()) {
            std::cout << " — " << testCase.message;
        }
        std::cout << '\n';
    }
    std::cout << lighting.summary << '\n';
    ok = ok && lighting.success;

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
