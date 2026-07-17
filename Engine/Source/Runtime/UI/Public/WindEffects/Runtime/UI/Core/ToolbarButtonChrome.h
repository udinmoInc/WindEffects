#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Geometry.h"

namespace WindEffects::Editor::UI {

class PaintContext;

namespace ToolbarButtonChrome {

UI_API float ButtonRadius(float uiScale);
UI_API float IconSize(float uiScale);
UI_API float PrimaryIconSize(float uiScale);
UI_API float HorizontalPad(float uiScale);
UI_API float ChipHorizontalPad(float uiScale);
UI_API float IconGapPx(float uiScale);
UI_API float ChevronGapPx(float uiScale);

UI_API Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx);

UI_API Color ResolveIconColor(float hoverAnim, float pressStrength, bool active);

UI_API void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale);

UI_API void PaintChipDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

UI_API void PaintViewportChip(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

} // namespace ToolbarButtonChrome

} // namespace WindEffects::Editor::UI
