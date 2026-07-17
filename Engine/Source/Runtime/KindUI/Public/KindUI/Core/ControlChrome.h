#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IThemeProvider.h"
#include "KindUI/Core/Geometry.h"

#include <string>

namespace we::runtime::kindui {

class PaintContext;

// Shared themed painters for design-system controls.
// All colors/metrics come from ThemeManager / ResolvedStyle — no hardcoded hex.
namespace ControlChrome {

struct InteractionState {
    float hoverAnim = 0.0f;
    float pressAnim = 0.0f;
    bool selected = false;
    bool focused = false;
    bool disabled = false;
};

[[nodiscard]] KINDUI_API ResolvedStyle Role(StyleRole role);
[[nodiscard]] KINDUI_API float HoverDamping();
[[nodiscard]] KINDUI_API float PressDamping();

KINDUI_API void PaintElevation(
    PaintContext& context,
    const Rect& rect,
    int elevation,
    float radius);

KINDUI_API void PaintFocusRing(
    PaintContext& context,
    const Rect& rect,
    float radius);

KINDUI_API void PaintFilledButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state,
    StyleRole hoverRole = StyleRole::ButtonHover,
    StyleRole pressRole = StyleRole::ButtonActive);

KINDUI_API void PaintGhostButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state);

KINDUI_API void PaintIconButtonFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    bool active = false);

KINDUI_API void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

KINDUI_API void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

KINDUI_API void PaintCard(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

KINDUI_API void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    const std::string& subtitle = {});

KINDUI_API void PaintCenteredLabel(
    PaintContext& context,
    const Rect& rect,
    const std::string& text,
    const Color& color,
    float fontSize,
    bool bold = false);

KINDUI_API void PaintDangerButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state);

KINDUI_API void PaintPopupSurface(
    PaintContext& context,
    const Rect& rect);

KINDUI_API void PaintTooltipSurface(
    PaintContext& context,
    const Rect& rect);

KINDUI_API void PaintCheckbox(
    PaintContext& context,
    const Rect& box,
    bool checked,
    const InteractionState& state);

} // namespace ControlChrome

} // namespace we::runtime::kindui
