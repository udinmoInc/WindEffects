#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Geometry.h"

namespace WindEffects::Editor::UI {

class PaintContext;

namespace ToolbarButtonChrome {

UIFRAMEWORK_API float ButtonRadius(float uiScale);
UIFRAMEWORK_API float IconSize(float uiScale);
UIFRAMEWORK_API float PrimaryIconSize(float uiScale);
UIFRAMEWORK_API float HorizontalPad(float uiScale);
UIFRAMEWORK_API float ChipHorizontalPad(float uiScale);

UIFRAMEWORK_API Color ResolveIconColor(float hoverAnim, float pressStrength, bool active);

UIFRAMEWORK_API void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale);

UIFRAMEWORK_API void PaintChipDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

UIFRAMEWORK_API void PaintViewportChip(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale);

} // namespace ToolbarButtonChrome

} // namespace WindEffects::Editor::UI
