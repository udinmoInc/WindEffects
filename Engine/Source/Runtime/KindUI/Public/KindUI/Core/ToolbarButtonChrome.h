#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Core/WindIcon.h"

namespace we::runtime::kindui {

class PaintContext;

namespace ToolbarButtonChrome {

KINDUI_API float ButtonRadius(float uiScale);
KINDUI_API float IconSize(float uiScale);
KINDUI_API float PrimaryIconSize(float uiScale);
KINDUI_API float NavigationIconSize(float uiScale);
KINDUI_API float WindowControlIconSize(float uiScale);
KINDUI_API float HorizontalPad(float uiScale);
KINDUI_API float ChipHorizontalPad(float uiScale);
KINDUI_API float IconGapPx(float uiScale);
KINDUI_API float ChevronGapPx(float uiScale);

/// Square toolbar hit target (DPI-scaled IconButtonSize).
KINDUI_API float ItemSize(float uiScale);
/// Toolbar row content height aligned to ToolbarHeight token.
KINDUI_API float RowContentHeight(float uiScale);
KINDUI_API float GroupGap(float uiScale);
KINDUI_API float ItemGap(float uiScale);

KINDUI_API Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx);

/// Grayscale multiply tint: muted at rest, brighter on hover/active (preserves authored hue).
KINDUI_API Color ResolveIconColor(float hoverAnim, float pressStrength, bool active);
KINDUI_API Color ResolvePlayIconColor(float hoverAnim, float pressStrength, bool active);

/// Standalone floating toolbar icon — no hit-box fill. Hover only brightens the glyph (+ soft glow).
KINDUI_API void PaintFloatingIcon(
    PaintContext& context,
    WindIconRef icon,
    const Rect& controlBounds,
    float glyphPx,
    float hoverAnim,
    float pressStrength = 0.0f,
    bool active = false);

/// Icon-only controls: no background/border/pill. Kept for call-site compatibility (no-op).
KINDUI_API void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale);

/// Optional thin accent indicator for active toggle tools.
KINDUI_API void PaintActiveIndicator(
    PaintContext& context,
    const Rect& rect,
    float activeAnim,
    float uiScale);

/// Inline dropdown chip: no border/background at rest; hover/press only.
KINDUI_API void PaintInlineDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

/// Subtle shared background for execution/transport clusters only.
KINDUI_API void PaintExecutionCluster(
    PaintContext& context,
    const Rect& rect,
    float uiScale);

/// Flat toolbar button hover/press/active surfaces (no bevel, no accent underline).
KINDUI_API void PaintToolbarButtonSurface(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale);

/// Status bar dock control: text brighten on hover, accent underline when active.
KINDUI_API void PaintStatusBarControl(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    bool active,
    float uiScale);

KINDUI_API void PaintChipDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

KINDUI_API void PaintViewportChip(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

} // namespace ToolbarButtonChrome

} // namespace we::runtime::kindui
