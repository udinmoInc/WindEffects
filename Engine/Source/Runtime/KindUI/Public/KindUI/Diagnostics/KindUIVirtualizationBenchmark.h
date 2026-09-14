// ==============================================================================
// WindEffects — KindUI — KindUIVirtualizationBenchmark
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::kindui {

struct KINDUI_API VirtualizationScaleResult {
    uint32_t itemCount = 0;
    uint32_t activeWidgets = 0;
    uint32_t poolSize = 0;
    uint64_t createCount = 0;
    uint64_t recycleCount = 0;
    uint64_t estimatedMemoryBytes = 0;
    double layoutMicros = 0.0;
    double paintMicros = 0.0;
    double scrollFrameMicros = 0.0;
    uint32_t paintCommands = 0;
    uint32_t drawCommandPeak = 0;
    /// Headless path cannot touch GPU; reported as 0.
    uint64_t gpuUploads = 0;
};

struct KINDUI_API KindUIVirtualizationReport {
    std::vector<VirtualizationScaleResult> listScales;
    std::vector<VirtualizationScaleResult> treeScales;
    std::string summary;
};

/// Headless VirtualList / CompactTreeWidget scale bench (100 / 1k / 10k / 100k).
[[nodiscard]] KINDUI_API KindUIVirtualizationReport RunKindUIVirtualizationBenchmark(
    uint32_t scrollSteps = 48);

} // namespace we::runtime::kindui
