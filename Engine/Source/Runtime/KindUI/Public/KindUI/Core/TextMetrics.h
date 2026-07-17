#pragma once

#include "KindUI/Export.h"

#include <functional>
#include <string_view>

namespace we::runtime::kindui {

/// Shared text measurement helpers. All widgets use these instead of ad-hoc heuristics.
struct KINDUI_API TextMetrics {
    using MeasureFn = std::function<float(std::string_view text, float fontSize, bool bold)>;

    static void SetMeasureProvider(MeasureFn provider);

    [[nodiscard]] static float MeasureWidth(std::string_view text, float fontSize, bool bold = false);

    /// Redirects to MeasureWidth for backward compatibility.
    [[nodiscard]] static float EstimateWidth(std::string_view text, float fontSize);

    [[nodiscard]] static float CharWidth(float fontSize);
};

} // namespace we::runtime::kindui
