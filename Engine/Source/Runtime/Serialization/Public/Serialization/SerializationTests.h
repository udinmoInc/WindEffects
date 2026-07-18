#pragma once

#include "Serialization/Diagnostics.h"
#include "Serialization/Export.h"
#include "Serialization/ISerializer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::serialization {

struct SERIALIZATION_API SerializationTestCaseResult {
    std::string name;
    bool passed = false;
    std::string message;
};

struct SERIALIZATION_API SerializationTestReport {
    std::vector<SerializationTestCaseResult> cases;
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    bool success = false;
    std::string summary;
};

[[nodiscard]] SERIALIZATION_API SerializationTestReport RunSerializationTests();

struct SERIALIZATION_API SerializationBenchmarkSample {
    std::string name;
    double milliseconds = 0.0;
    std::uint64_t iterations = 0;
    double perIterationNs = 0.0;
};

struct SERIALIZATION_API SerializationBenchmarkConfig {
    std::uint32_t objectCount = 10000;
    std::uint32_t graphSize = 1000;
    bool runFullScale = false; // 1,000,000 objects
};

struct SERIALIZATION_API SerializationBenchmarkReport {
    std::vector<SerializationBenchmarkSample> samples;
    SerializationDiagnosticsSnapshot diagnostics{};
    bool success = false;
    std::string summary;
};

[[nodiscard]] SERIALIZATION_API SerializationBenchmarkReport RunSerializationBenchmarks(
    const SerializationBenchmarkConfig& config = {});

struct SERIALIZATION_API SerializationProductionReport {
    SerializationTestReport tests{};
    SerializationBenchmarkReport benchmark{};
    SerializationDiagnosticsSnapshot diagnostics{};
    bool productionReady = false;
    std::string verdict;
    std::string textReport;
    std::string jsonReport;
};

struct SERIALIZATION_API SerializationProductionReportConfig {
    bool runTests = true;
    bool runBenchmarks = true;
    bool runFullScaleBenchmarks = false;
    SerializationBenchmarkConfig benchmarkConfig{};
};

[[nodiscard]] SERIALIZATION_API SerializationProductionReport GenerateSerializationProductionReport(
    const SerializationProductionReportConfig& config = {});

[[nodiscard]] SERIALIZATION_API bool WriteSerializationProductionReportFiles(
    const SerializationProductionReport& report,
    std::string_view textPath,
    std::string_view jsonPath);

[[nodiscard]] SERIALIZATION_API SerializationDiagnosticsSnapshot CaptureSerializationDiagnostics();

} // namespace we::runtime::serialization
