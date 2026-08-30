#pragma once

#include <algorithm>

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::layout {

// UE5-style editor spacing rhythm — logical px before DPI.
namespace EditorSpacing {
[[nodiscard]] inline float XS() { return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::SpaceXS); }
[[nodiscard]] inline float SM() { return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space1); }
[[nodiscard]] inline float MD() { return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::SpaceMD); }
[[nodiscard]] inline float LG() { return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space2); }
[[nodiscard]] inline float XL() { return we::runtime::kindui::ResolveMetric(we::runtime::kindui::MetricToken::Space3); }
} // namespace EditorSpacing

// Centralized editor chrome metrics — all values are logical px; scale via Scaled().
namespace EditorMetrics {
using MetricToken = we::runtime::kindui::MetricToken;

[[nodiscard]] inline float UiScale() {
    return std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
}

[[nodiscard]] inline float Scaled(MetricToken token) {
    return we::runtime::kindui::ResolveMetric(token) * UiScale();
}

[[nodiscard]] inline float Scaled(float logicalPx) {
    return logicalPx * UiScale();
}

[[nodiscard]] inline float ToolbarHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::ToolbarHeight); }
[[nodiscard]] inline float TabHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight); }
[[nodiscard]] inline float PanelHeaderHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::PanelHeaderHeight); }
[[nodiscard]] inline float SearchHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::SearchBoxHeight); }
[[nodiscard]] inline float TreeRowHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight); }
[[nodiscard]] inline float PropertyRowHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::FormRowHeight); }
[[nodiscard]] inline float CategoryHeaderHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::CategoryHeaderHeight); }
[[nodiscard]] inline float StatusBarHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::StatusBarHeight); }
[[nodiscard]] inline float ControlHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::ButtonHeight); }
[[nodiscard]] inline float ViewportToolbarHeight() { return we::runtime::kindui::ResolveMetric(MetricToken::ViewportToolbarHeight); }
[[nodiscard]] inline float IconSize() { return we::runtime::kindui::ResolveMetric(MetricToken::IconSizeTree); }
[[nodiscard]] inline float LargeIconSize() { return we::runtime::kindui::ResolveMetric(MetricToken::IconSizeToolbar); }
[[nodiscard]] inline float TreeIndent() { return we::runtime::kindui::ResolveMetric(MetricToken::TreeIndentWidth); }
[[nodiscard]] inline float TreeExpanderHitSize() { return we::runtime::kindui::ResolveMetric(MetricToken::TreeExpanderHitSize); }
[[nodiscard]] inline float TreeExplorerPrefixWidth() {
    return we::runtime::kindui::ResolveMetric(MetricToken::Space2)
        + 2.0f * we::runtime::kindui::ResolveMetric(MetricToken::TreeExpanderHitSize);
}
[[nodiscard]] inline float TabPaddingH() { return we::runtime::kindui::ResolveMetric(MetricToken::TabPaddingH); }
[[nodiscard]] inline float TabPaddingV() { return we::runtime::kindui::ResolveMetric(MetricToken::TabPaddingV); }

/// Vertically center `contentHeight` inside `[containerY, containerY + containerHeight]`.
[[nodiscard]] inline float CenterY(float containerY, float containerHeight, float contentHeight) {
    return containerY + (containerHeight - contentHeight) * 0.5f;
}

} // namespace EditorMetrics

} // namespace we::editor::layout
