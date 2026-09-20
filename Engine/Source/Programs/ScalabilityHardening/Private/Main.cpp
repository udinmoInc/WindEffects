// ==============================================================================
// WindEffects — ScalabilityHardening — Main
// Runs Scalability runtime tests (profile resolve, capability fallback, switching).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityTests.h"
#include "Lighting/LightingTests.h"
#include "Renderer/Volumetrics/VolumetricTests.h"

#include <cstdio>
#include <cstdlib>

int main(int /*argc*/, char** /*argv*/) {
    std::printf("ScalabilityHardening starting...\n");
    std::fflush(stdout);

    using we::runtime::renderer::RunScalabilityRuntimeTests;
    using we::runtime::renderer::RunLightingRuntimeTests;
    using we::runtime::renderer::RunVolumetricFoundationTests;

    bool ok = true;

    std::printf("Running Scalability runtime tests...\n");
    std::fflush(stdout);
    const auto scalability = RunScalabilityRuntimeTests();
    for (const auto& testCase : scalability.cases) {
        std::printf("%s %s", testCase.passed ? "[PASS]" : "[FAIL]", testCase.name.c_str());
        if (!testCase.message.empty()) {
            std::printf(" — %s", testCase.message.c_str());
        }
        std::printf("\n");
    }
    std::printf("%s\n", scalability.summary.c_str());
    ok = ok && scalability.success;

    std::printf("\nRunning Lighting runtime tests...\n");
    std::fflush(stdout);
    const auto lighting = RunLightingRuntimeTests();
    for (const auto& testCase : lighting.cases) {
        std::printf("%s %s", testCase.passed ? "[PASS]" : "[FAIL]", testCase.name.c_str());
        if (!testCase.message.empty()) {
            std::printf(" — %s", testCase.message.c_str());
        }
        std::printf("\n");
    }
    std::printf("%s\n", lighting.summary.c_str());
    ok = ok && lighting.success;

    std::printf("\nRunning Volumetric foundation tests...\n");
    std::fflush(stdout);
    const auto volumetrics = RunVolumetricFoundationTests();
    for (const auto& testCase : volumetrics.cases) {
        std::printf("%s %s", testCase.passed ? "[PASS]" : "[FAIL]", testCase.name.c_str());
        if (!testCase.message.empty()) {
            std::printf(" — %s", testCase.message.c_str());
        }
        std::printf("\n");
    }
    std::printf("%s\n", volumetrics.summary.c_str());
    ok = ok && volumetrics.success;

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
