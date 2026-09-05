#pragma once

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::runtime::kindui::ChromeSeparation {

/// Global editor chrome policy: separate adjacent surfaces with a tiny gap so the
/// darker workspace background shows through. No drawn borders, outlines, or separator lines.
inline constexpr bool kGapCutsEnabled = true;

[[nodiscard]] inline float Gap() {
    return ResolveMetric(MetricToken::ChromeSeparationGap);
}

[[nodiscard]] inline float GapWide() {
    return ResolveMetric(MetricToken::ChromeSeparationGapWide);
}

/// Vertical toolbar / panel-toolbar divider width (not title/workspace chrome gaps).
[[nodiscard]] inline float DividerWidth() {
    return ResolveMetric(MetricToken::ToolbarSeparatorWidth);
}

/// Logical dock gutter per edge when panels inset their own chrome.
/// With gap-cuts enabled, gutters come from per-panel structure inset + workspace padding.
[[nodiscard]] inline float DockGutterGap() {
    if (kGapCutsEnabled) {
        return 0.0f;
    }
    return ResolveMetric(MetricToken::DockPanelGap) * 0.5f;
}

/// Device-pixel gutter width for internal panel region separation.
[[nodiscard]] inline float GapDevicePx() {
    if (!kGapCutsEnabled) {
        return 0.0f;
    }
    return DPIContext::Snap(Gap() * DPIContext::GetScale());
}

/// Device-pixel dock panel gutter per edge (left/right/top/bottom).
/// With gap-cuts, workspace padding and splitters provide clean 3px separation.
[[nodiscard]] inline float DockStructureGapPx() {
    return 0.0f;
}

/// Device-pixel gutter drawn by dock splitters between adjacent panels.
[[nodiscard]] inline float DockSplitterGapPx() {
    if (!kGapCutsEnabled) {
        return 0.0f;
    }
    return (std::max)(1.0f, GapDevicePx());
}

} // namespace we::runtime::kindui::ChromeSeparation
