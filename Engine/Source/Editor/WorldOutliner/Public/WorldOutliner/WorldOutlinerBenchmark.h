#pragma once

#include "WorldOutliner/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::outliner {

struct WORLDOUTLINER_API WorldOutlinerBenchmarkReport {
    std::uint64_t entityCount = 0;
    std::uint64_t rebuildIterations = 0;
    std::uint64_t rebuildTotalMicros = 0;
    std::uint64_t filterIterations = 0;
    std::uint64_t filterTotalMicros = 0;
    std::string summary;
};

[[nodiscard]] WORLDOUTLINER_API WorldOutlinerBenchmarkReport RunWorldOutlinerBenchmarks(
    std::uint32_t entityCount = 5000,
    std::uint32_t iterations = 50);

} // namespace we::editor::outliner
