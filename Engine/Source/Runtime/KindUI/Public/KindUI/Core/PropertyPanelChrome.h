#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

#include <string>
#include <string_view>

namespace we::runtime::kindui {

class PaintContext;
class Widget;

/// Shared layout and paint primitives for Details / property inspector panels.
namespace PropertyPanelChrome {

struct PropertyRowLayout {
    Rect row;
    Rect label;
    Rect value;
};

[[nodiscard]] KINDUI_API float ObjectHeaderHeight();
[[nodiscard]] KINDUI_API float CategoryTabHeight();
[[nodiscard]] KINDUI_API float CategoryTabRowHeight();
[[nodiscard]] KINDUI_API float SectionHeight();
[[nodiscard]] KINDUI_API float RowHeight();
[[nodiscard]] KINDUI_API float LabelColumnWidth();
[[nodiscard]] KINDUI_API float RowPaddingH();
[[nodiscard]] KINDUI_API float ControlPaddingH();
[[nodiscard]] KINDUI_API float PropertyIndentStep();

/// Apply label/control flex sizing from LayoutPropertyRow metrics.
KINDUI_API void ConfigureFormRowChildren(Widget& labelWidget, Widget* controlWidget, int depth = 0);

[[nodiscard]] KINDUI_API float FormColumnPadding();
[[nodiscard]] KINDUI_API float FormStackGap();
[[nodiscard]] KINDUI_API float ValueColumnGap();

[[nodiscard]] KINDUI_API PropertyRowLayout LayoutPropertyRow(const Rect& rowRect, int depth = 0);
[[nodiscard]] KINDUI_API Rect LayoutPropertyControlRect(const Rect& valueRect);

KINDUI_API void PaintObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    std::string_view iconName,
    bool active);

KINDUI_API void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    bool expanded,
    bool hovered,
    float indent = 0.0f);

KINDUI_API void PaintPropertyRowLabel(
    PaintContext& context,
    const Rect& labelRect,
    std::string_view label,
    bool mixed);

KINDUI_API void PaintCategoryTab(
    PaintContext& context,
    const Rect& rect,
    std::string_view label,
    bool active,
    bool hovered);

KINDUI_API void PaintPropertyRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    bool hovered,
    bool selected);

} // namespace PropertyPanelChrome

} // namespace we::runtime::kindui
