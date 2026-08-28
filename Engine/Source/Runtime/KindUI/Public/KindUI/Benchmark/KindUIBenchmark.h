#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>

namespace we::runtime::kindui {

struct KINDUI_API KindUIBenchmarkReport {
    uint32_t widgetCount = 0;
    uint32_t iterations = 0;

    double idleFrameMicros = 0.0;
    double hoverFrameMicros = 0.0;
    double layoutMicros = 0.0;
    double paintMicros = 0.0;
    double fullRebuildMicros = 0.0;

    uint64_t idleSkips = 0;
    uint64_t paintRebuilds = 0;
    uint64_t layoutRebuilds = 0;

    std::string summary;
};

/// Headless KindUI framework benchmark (layout, paint, invalidation gates).
[[nodiscard]] KINDUI_API KindUIBenchmarkReport RunKindUIBenchmark(uint32_t iterations = 500);

} // namespace we::runtime::kindui
