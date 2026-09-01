#pragma once

#include "KindUI/Export.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui::ChromeSeparation {

/// Global editor chrome policy: separate adjacent surfaces with a tiny gap so the
/// darker workspace background shows through. No drawn borders, outlines, or separator lines.
inline constexpr bool kGapCutsEnabled = true;

[[nodiscard]] KINDUI_API inline float Gap() {
    return ResolveMetric(MetricToken::ChromeSeparationGap);
}

[[nodiscard]] KINDUI_API inline float GapWide() {
    return ResolveMetric(MetricToken::ChromeSeparationGapWide);
}

} // namespace we::runtime::kindui::ChromeSeparation
