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
/// Each dock container insets its chrome so workspace background shows through.
[[nodiscard]] inline float DockStructureGapPx() {
    if (!kGapCutsEnabled) {
        return 0.0f;
    }
    return GapDevicePx();
}

/// Device-pixel gutter drawn by dock splitters between adjacent panels.
[[nodiscard]] inline float DockSplitterGapPx() {
    if (!kGapCutsEnabled) {
        return 0.0f;
    }
    // Per-panel structure gaps already separate adjacent dock chrome; avoid double gutters.
    if (DockStructureGapPx() > 0.0f) {
        return 0.0f;
    }
    return (std::max)(1.0f, GapDevicePx());
}

} // namespace we::runtime::kindui::ChromeSeparation
