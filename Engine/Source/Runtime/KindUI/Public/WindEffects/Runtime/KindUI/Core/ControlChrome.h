#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Core/Geometry.h"

#include <string>

namespace WindEffects::Editor::UI {

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

[[nodiscard]] UI_API ResolvedStyle Role(StyleRole role);
[[nodiscard]] UI_API float HoverDamping();
[[nodiscard]] UI_API float PressDamping();

UI_API void PaintElevation(
    PaintContext& context,
    const Rect& rect,
    int elevation,
    float radius);

UI_API void PaintFocusRing(
    PaintContext& context,
    const Rect& rect,
    float radius);

UI_API void PaintFilledButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state,
    StyleRole hoverRole = StyleRole::ButtonHover,
    StyleRole pressRole = StyleRole::ButtonActive);

UI_API void PaintGhostButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state);

UI_API void PaintIconButtonFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    bool active = false);

UI_API void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

UI_API void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

UI_API void PaintCard(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

UI_API void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    const std::string& subtitle = {});

UI_API void PaintCenteredLabel(
    PaintContext& context,
    const Rect& rect,
    const std::string& text,
    const Color& color,
    float fontSize,
    bool bold = false);

UI_API void PaintDangerButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state);

UI_API void PaintPopupSurface(
    PaintContext& context,
    const Rect& rect);

UI_API void PaintTooltipSurface(
    PaintContext& context,
    const Rect& rect);

UI_API void PaintCheckbox(
    PaintContext& context,
    const Rect& box,
    bool checked,
    const InteractionState& state);

} // namespace ControlChrome

} // namespace WindEffects::Editor::UI
