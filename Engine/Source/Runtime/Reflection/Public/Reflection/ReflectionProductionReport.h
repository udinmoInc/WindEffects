#pragma once

#include "Reflection/Export.h"
#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/ReflectionBenchmark.h"
#include "Reflection/ReflectionValidation.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

struct REFLECTION_API ReflectionTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct REFLECTION_API ReflectionTestReport {
    std::vector<ReflectionTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

/// Comprehensive automated suite covering registration, plugins, queries, validation,
/// migration, concurrency, and memory correctness. Uses an isolated registry.
[[nodiscard]] REFLECTION_API ReflectionTestReport RunReflectionTests();

struct REFLECTION_API ProductionReadinessAssessment {
    bool productionReady = false;
    bool abiStable = false;
    bool binaryCompatible = false;
    bool validationClean = false;
    bool cachesValid = false;
    bool pluginUnloadSafe = true;
    std::string verdict;
    std::vector<std::string> remainingTechnicalDebt;
    std::vector<std::string> scalabilityNotes;
};

struct REFLECTION_API ReflectionProductionReport {
    ReflectionDiagnosticsSnapshot diagnostics{};
    CacheVerificationReport caches{};
    BenchmarkReport benchmark{};
    ReflectionTestReport tests{};
    ProductionReadinessAssessment assessment{};
    std::string textReport;
    std::string jsonReport;
};

struct REFLECTION_API ProductionReportConfig {
    bool runTests = true;
    bool runBenchmarks = true;
    bool runFullScaleBenchmarks = false; // 100k types / 1M objects / 100 plugins
    BenchmarkConfig benchmarkConfig{};
};

/// Generate the final production hardening report (in-memory text + JSON).
[[nodiscard]] REFLECTION_API ReflectionProductionReport GenerateProductionReport(
    const ProductionReportConfig& config = {});

/// Write textReport / jsonReport to files. Returns false on I/O failure.
[[nodiscard]] REFLECTION_API bool WriteProductionReportFiles(
    const ReflectionProductionReport& report,
    std::string_view textPath,
    std::string_view jsonPath);

} // namespace we::runtime::reflection
