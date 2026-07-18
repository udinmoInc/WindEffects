#pragma once

#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/Export.h"
#include "Reflection/ITypeRegistry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

struct REFLECTION_API BenchmarkSample {
    std::string name;
    double milliseconds = 0.0;
    std::uint64_t iterations = 0;
    double perIterationNs = 0.0;
    std::size_t bytesObserved = 0;
};

struct REFLECTION_API BenchmarkConfig {
    std::uint32_t typeCount = 10000;          // scale toward 100000 in full runs
    std::uint32_t objectsPerType = 10;        // typeCount * objectsPerType toward 1e6 objects
    std::uint32_t pluginCount = 10;           // scale toward 100
    std::uint32_t inheritanceDepth = 8;
    std::uint32_t propertiesPerType = 8;
    bool runFullScale = false;               // when true: 100k types / 1M objects / 100 plugins
};

struct REFLECTION_API BenchmarkReport {
    std::vector<BenchmarkSample> samples;
    ReflectionDiagnosticsSnapshot diagnostics{};
    bool success = false;
    std::string summary;
};

/// Runs in-process microbenchmarks + optional scale test against a fresh registry.
[[nodiscard]] REFLECTION_API BenchmarkReport RunReflectionBenchmarks(
    const BenchmarkConfig& config = {});

} // namespace we::runtime::reflection
