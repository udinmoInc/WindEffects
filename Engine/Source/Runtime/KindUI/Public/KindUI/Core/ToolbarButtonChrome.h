#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

namespace we::runtime::kindui {

class PaintContext;

namespace ToolbarButtonChrome {

KINDUI_API float ButtonRadius(float uiScale);
KINDUI_API float IconSize(float uiScale);
KINDUI_API float PrimaryIconSize(float uiScale);
KINDUI_API float HorizontalPad(float uiScale);
KINDUI_API float ChipHorizontalPad(float uiScale);
KINDUI_API float IconGapPx(float uiScale);
KINDUI_API float ChevronGapPx(float uiScale);

KINDUI_API Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx);

KINDUI_API Color ResolveIconColor(float hoverAnim, float pressStrength, bool active);

KINDUI_API void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
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
