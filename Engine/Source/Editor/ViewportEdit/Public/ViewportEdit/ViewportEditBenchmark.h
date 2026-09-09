// ==============================================================================
// WindEffects — ViewportEdit — ViewportEditBenchmark
// Public API surface for the ViewportEdit module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ViewportEdit/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::viewportedit {

struct VIEWPORTEDIT_API ViewportEditBenchmarkReport {
    std::uint64_t pickIterations = 0;
    std::uint64_t pickTotalMicros = 0;
    std::uint64_t transformIterations = 0;
    std::uint64_t transformTotalMicros = 0;
    std::string summary;
};

[[nodiscard]] VIEWPORTEDIT_API ViewportEditBenchmarkReport RunViewportEditBenchmarks(
    std::uint32_t entityCount = 1000,
    std::uint32_t iterations = 500);

} // namespace we::editor::viewportedit
