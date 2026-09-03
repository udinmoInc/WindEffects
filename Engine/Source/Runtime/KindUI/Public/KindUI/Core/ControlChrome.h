#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ResolvedStyle.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Tokens/SurfaceRole.h"

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

KINDUI_API void PaintPopupShadow(PaintContext& context, const Rect& rect, float radius = 4.0f);

KINDUI_API void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    SurfaceRole surfaceRole = SurfaceRole::Panel);

KINDUI_API void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken);

/// Soft drop shadow for raised controls (buttons, chips).
KINDUI_API void PaintSubtleDropShadow(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float strength = 1.0f);

/// Top/left highlight + bottom/right shade for a raised 3D control face.
KINDUI_API void PaintRaisedBevel(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float strength = 1.0f);

/// Single 1px top inner highlight for recessed inputs (no side or bottom rims).
KINDUI_API void PaintInsetBevel(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float strength = 1.0f);

/// Soft outer edge depth for panels, cards, and region borders.
KINDUI_API void PaintSubtleBorderDepth(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float strength = 1.0f);

/// UE Slate toolbar/panel button edge — top highlight + bottom shade.
KINDUI_API void PaintSlateButtonBevel(
    PaintContext& context,
    const Rect& rect,
    float strength = 1.0f);

/// Raised 3D face for in-panel buttons and tabs (shadow, border, bevel).
KINDUI_API void PaintPanelButtonFace(
    PaintContext& context,
    const Rect& rect,
    const Color& background,
    float radius,
    float hoverAnim = 0.0f,
    float pressAnim = 0.0f,
    bool emphasized = false);

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

/// UE panel-toolbar style: icon only, hover/press tint with no persistent border.
KINDUI_API void PaintBorderlessIconButton(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

KINDUI_API void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

/// Recessed console field integrated into the editor status bar (UE footer style).
KINDUI_API void PaintStatusBarCommandField(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

KINDUI_API void PaintSearchInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state);

struct SearchFieldPaintOptions {
    bool showClearButton = false;
    bool clearHovered = false;
    /// Panel toolbar search: no recessed input chrome or focus outline.
    bool toolbarFlat = false;
};

KINDUI_API void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    const InteractionState& state,
    bool showCaret = false,
    const SearchFieldPaintOptions& options = {});

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

/// In-panel workspace tab (Create / Sculpt / …). No strip background — inactive tabs are transparent.
KINDUI_API void PaintPanelTab(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    const InteractionState& state);

/// Draws a connected vertical editor divider line across [top, bottom].
KINDUI_API void PaintVerticalSeparator(
    PaintContext& context,
    float x,
    float top,
    float bottom,
    float thickness = 1.0f,
    ColorToken colorToken = ColorToken::Separator);

} // namespace ControlChrome

} // namespace we::runtime::kindui
