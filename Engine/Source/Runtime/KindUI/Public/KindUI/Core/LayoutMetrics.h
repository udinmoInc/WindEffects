#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace we::runtime::kindui {

class Row;
class Column;

namespace LayoutMetrics {

/// Resolved layout height for a style role (DPI-scaled, matches StyleResolver).
[[nodiscard]] KINDUI_API float ResolvedStyleHeight(StyleRole role, MetricToken fallbackToken);

[[nodiscard]] KINDUI_API float InputMinHeight();
[[nodiscard]] KINDUI_API float SearchInputHeight();
[[nodiscard]] KINDUI_API float ToolbarSearchInputHeight();
[[nodiscard]] KINDUI_API float SearchRowHeight();
[[nodiscard]] KINDUI_API float SearchInputPaddingH();
[[nodiscard]] KINDUI_API float SearchInputFontSize();
[[nodiscard]] KINDUI_API float SearchInputIconSize();
[[nodiscard]] KINDUI_API Rect LayoutSearchInputRect(const Rect& allottedRect);
[[nodiscard]] KINDUI_API Rect LayoutToolbarSearchInputRect(const Rect& allottedRect);
[[nodiscard]] KINDUI_API float FormRowMinHeight();
[[nodiscard]] KINDUI_API float PropertySectionHeight();
[[nodiscard]] KINDUI_API float PropertyObjectHeaderHeight();
[[nodiscard]] KINDUI_API float PropertyCategoryTabRowHeight();
[[nodiscard]] KINDUI_API float PropertyLabelColumnWidth();
[[nodiscard]] KINDUI_API float PropertyRowHeight();
[[nodiscard]] KINDUI_API float PropertyControlHeight();
[[nodiscard]] KINDUI_API Rect LayoutPropertyControlInRow(const Rect& valueRect);
[[nodiscard]] KINDUI_API float ButtonMinHeight(StyleRole role = StyleRole::ButtonSecondary);

/// Enforce minimum interactive sizes so flex parents cannot compress controls.
KINDUI_API void ApplyInputMinSize(Widget& widget);
KINDUI_API void ApplyFormRowMinSize(Widget& widget);
KINDUI_API void ApplyButtonMinSize(Widget& widget, StyleRole role = StyleRole::ButtonSecondary);

/// Standard property/settings row: fixed label column + growing control.
[[nodiscard]] KINDUI_API std::shared_ptr<Row> MakeFormRow(
    const std::string& label,
    const std::shared_ptr<Widget>& control);

/// Text input form row wired to a commit callback.
[[nodiscard]] KINDUI_API std::shared_ptr<Row> MakeTextFormRow(
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit);

/// Stretch + inset for scrollable property/form columns.
KINDUI_API void ConfigurePropertyFormColumn(Column& column);

/// Minimum width for equal-width chip/action rows in tool panels.
[[nodiscard]] KINDUI_API float FormChipButtonMinWidth();

/// Roboto wefont face metrics: lineHeight / bakeSize (32 / 24).
inline constexpr float kTextLineHeightRatio = 32.0f / 24.0f;

/// Layout line box height for a DrawText fontSize (matches TextLayoutEngine).
[[nodiscard]] KINDUI_API float TextLineHeight(float fontSizePx);

/// Top Y for PaintContext::DrawText so the line box is vertically centered in bounds.
[[nodiscard]] KINDUI_API float AlignTextTopY(const Rect& bounds, float fontSizePx);

/// Top Y for DrawText centered on a horizontal midline (toolbar / tab chrome).
[[nodiscard]] KINDUI_API float AlignTextTopAtCenterY(float centerY, float fontSizePx);

} // namespace LayoutMetrics
} // namespace we::runtime::kindui
