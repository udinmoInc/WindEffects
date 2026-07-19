#pragma once

#include "World/Export.h"
#include "World/WorldDiagnostics.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::world {

struct WORLD_API WorldBenchmarkSample {
    std::string name;
    double milliseconds = 0.0;
    std::uint64_t iterations = 0;
    double perIterationNs = 0.0;
};

struct WORLD_API WorldBenchmarkConfig {
    std::uint32_t actorCount = 10000;
    std::uint32_t queryIterations = 1000;
    bool runStress = false;
};

struct WORLD_API WorldBenchmarkReport {
    std::vector<WorldBenchmarkSample> samples;
    WorldDiagnosticsSnapshot diagnostics{};
    bool success = false;
    std::string summary;
};

[[nodiscard]] WORLD_API WorldBenchmarkReport RunWorldRuntimeBenchmarks(
    const WorldBenchmarkConfig& config = {});

} // namespace we::runtime::world
