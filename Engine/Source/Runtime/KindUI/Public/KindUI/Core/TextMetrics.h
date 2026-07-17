#pragma once

#include "KindUI/Export.h"

#include <string_view>

namespace we::runtime::kindui {

/// Shared text measurement helpers. All widgets use these instead of ad-hoc heuristics.
struct KINDUI_API TextMetrics {
    [[nodiscard]] static float EstimateWidth(std::string_view text, float fontSize);
    [[nodiscard]] static float CharWidth(float fontSize);
};

} // namespace we::runtime::kindui
