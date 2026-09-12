// ==============================================================================
// WindEffects — KindUI — PropertyPanelChrome
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

#include <string>
#include <string_view>

namespace we::runtime::kindui {

class PaintContext;
class Widget;

namespace PropertyPanelChrome {

/// Which trailing property actions are applicable for a row.
struct PropertyActionIcons {
    bool undo = false;
    /// When true, Undo paints enabled; when false it stays clearly disabled.
    bool undoEnabled = false;
    /// Deprecated: options (⋮) removed from Inspector rows; kept for ABI stability.
    bool options = false;

    [[nodiscard]] bool Any() const noexcept { return undo; }
};

/// Fixed trailing slots: undo. Empty slots stay reserved so the
/// value column never shifts when an action is hidden.
struct PropertyActionLayout {
    Rect strip;
    Rect undo;
    Rect options;
};

struct PropertyRowLayout {
    Rect row;
    Rect label;
    Rect value;
    Rect actions;
    Rect lockIcon; // Position for lock icon next to label
    /// X center of the label|value divider (already snapped for paint).
    float columnDividerX = 0.0f;
    /// X center of the value|actions divider (already snapped for paint).
    float valueRightDividerX = 0.0f;
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

KINDUI_API void ConfigureFormRowChildren(Widget& labelWidget, Widget* controlWidget, int depth = 0);

[[nodiscard]] KINDUI_API float FormColumnPadding();
[[nodiscard]] KINDUI_API float FormStackGap();
[[nodiscard]] KINDUI_API float ValueColumnGap();
/// Compact glyph size for Inspector trailing actions (undo / options).
[[nodiscard]] KINDUI_API float ActionIconSize();
/// Gap between reserved action slots (matches divider inset / vector pad).
[[nodiscard]] KINDUI_API float ActionSlotGap();
/// Trailing strip width based on which actions are present.
[[nodiscard]] KINDUI_API float ActionStripWidth(const PropertyActionIcons& icons);
/// Optional end inset for shortened divider segments (0 = continuous edge-to-edge).
[[nodiscard]] KINDUI_API float DividerInset();

[[nodiscard]] KINDUI_API PropertyRowLayout LayoutPropertyRow(const Rect& rowRect, int depth = 0,
    const PropertyActionIcons& icons = {}, bool hasLockIcon = false, float labelColumnRatio = 0.40f);
[[nodiscard]] KINDUI_API Rect LayoutPropertyControlRect(const Rect& valueRect);
[[nodiscard]] KINDUI_API PropertyActionLayout LayoutPropertyActions(const Rect& actionsRect);

/// Paint highlight on vertical column divider when splitter is hovered or dragged.
KINDUI_API void PaintPropertySplitterHighlight(
    PaintContext& context,
    float x,
    float top,
    float bottom,
    bool isDragging,
    bool isHovered);

/// Draw the persistent, theme-owned edge around an Inspector value cell.
KINDUI_API void PaintPropertyControlBorder(PaintContext& context, const Rect& valueRect);
/// Vertical rule using BorderSubtle (dark input outline — not light, not near-black).
KINDUI_API void PaintPropertyDivider(
    PaintContext& context,
    float x,
    float top,
    float bottom);
/// Horizontal rule using the same BorderSubtle stroke.
KINDUI_API void PaintPropertyHorizontalDivider(
    PaintContext& context,
    float y,
    float left,
    float right);
/// Continuous full-height label|value column rule for one property row.
KINDUI_API void PaintPropertyColumnDivider(PaintContext& context, const PropertyRowLayout& layout);
KINDUI_API void PaintPropertyRowGrid(PaintContext& context, const PropertyRowLayout& layout);
/// Paint trailing actions. Undo uses a persistent enabled/disabled tint (not hover).
KINDUI_API void PaintPropertyActions(
    PaintContext& context,
    const PropertyActionLayout& layout,
    const PropertyActionIcons& icons,
    float optionsHover = 0.0f);

KINDUI_API void PaintObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    WindIconRef icon,
    bool active);

/// Compact Details actor header (inline 16px icon, no redundant instance row).
KINDUI_API void PaintDetailsObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view displayName,
    WindIconRef icon);

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

/// Paint lock icon next to property label.
KINDUI_API void PaintPropertyLockIcon(
    PaintContext& context,
    const Rect& lockIconRect,
    bool locked);

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

}

}
