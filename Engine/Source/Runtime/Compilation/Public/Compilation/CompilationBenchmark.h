#pragma once

#include "Compilation/Export.h"

#include <cstdint>
#include <string>

namespace we::runtime::compilation {

struct COMPILATION_API CompilationBenchmarkReport {
    std::uint64_t syncIterations = 0;
    std::uint64_t syncTotalMicros = 0;
    std::uint64_t cacheHitIterations = 0;
    std::uint64_t cacheHitTotalMicros = 0;
    std::uint64_t parallelJobs = 0;
    std::uint64_t parallelTotalMicros = 0;
    std::string summary;
};

[[nodiscard]] COMPILATION_API CompilationBenchmarkReport RunCompilationBenchmarks(
    std::uint32_t syncIterations = 200,
    std::uint32_t parallelJobs = 64);

} // namespace we::runtime::compilation
