#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <memory>
#include <string>

namespace we::runtime::kindui {

class Row;

namespace LayoutMetrics {

/// Resolved layout height for a style role (DPI-scaled, matches StyleResolver).
[[nodiscard]] KINDUI_API float ResolvedStyleHeight(StyleRole role, MetricToken fallbackToken);

[[nodiscard]] KINDUI_API float InputMinHeight();
[[nodiscard]] KINDUI_API float FormRowMinHeight();
[[nodiscard]] KINDUI_API float ButtonMinHeight(StyleRole role = StyleRole::ButtonSecondary);

/// Enforce minimum interactive sizes so flex parents cannot compress controls.
KINDUI_API void ApplyInputMinSize(Widget& widget);
KINDUI_API void ApplyFormRowMinSize(Widget& widget);
KINDUI_API void ApplyButtonMinSize(Widget& widget, StyleRole role = StyleRole::ButtonSecondary);

/// Standard property/settings row: fixed label column + growing control.
[[nodiscard]] KINDUI_API std::shared_ptr<Row> MakeFormRow(
    const std::string& label,
    const std::shared_ptr<Widget>& control);

} // namespace LayoutMetrics
} // namespace we::runtime::kindui
