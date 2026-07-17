#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include <string_view>

namespace WindEffects::Editor::UI {

/// Shared text measurement helpers. All widgets use these instead of ad-hoc heuristics.
struct UI_API TextMetrics {
    [[nodiscard]] static float EstimateWidth(std::string_view text, float fontSize);
    [[nodiscard]] static float CharWidth(float fontSize);
};

} // namespace WindEffects::Editor::UI
