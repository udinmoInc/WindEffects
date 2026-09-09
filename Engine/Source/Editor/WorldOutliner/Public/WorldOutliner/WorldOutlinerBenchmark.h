// ==============================================================================
// WindEffects — WorldOutliner — WorldOutlinerBenchmark
// Public API surface for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
