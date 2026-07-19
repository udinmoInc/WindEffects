#pragma once

#include "Prefab/Export.h"

#include <cstdint>
#include <string>

namespace we::runtime::prefab {

struct PREFAB_API PrefabBenchmarkReport {
    std::uint64_t spawnIterations = 0;
    std::uint64_t spawnTotalMicros = 0;
    std::uint64_t serializeIterations = 0;
    std::uint64_t serializeTotalMicros = 0;
    std::string summary;
};

[[nodiscard]] PREFAB_API PrefabBenchmarkReport RunPrefabBenchmarks(
    std::uint32_t spawnIterations = 200,
    std::uint32_t serializeIterations = 100);

} // namespace we::runtime::prefab
