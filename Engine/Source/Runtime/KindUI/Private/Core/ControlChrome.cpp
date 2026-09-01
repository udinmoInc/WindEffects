#include "KindUI/Core/ControlChrome.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignSystem.h"

#include <algorithm>
#include <string>
#include <string_view>


namespace we::runtime::kindui {
namespace ControlChrome {
using ::we::runtime::kindui::kWindIconNone;

namespace {

enum class ControlBorderMode {
    None,
    Subtle,
    Styled,
};

struct ControlFrameStyle {
    Color normalBackground = Color::Transparent();
    Color hoverBackground = Color::Transparent();
    Color pressedBackground = Color::Transparent();
    Color disabledBackground = Color::Transparent();
    Color selectedBackground = Color::Transparent();
    float cornerRadius = 0.0f;
};

struct ResolvedControlBorder {
    Color color = Color::Transparent();
    float width = 0.0f;
};

Color ResolveControlBackground(const InteractionState& state, const ControlFrameStyle& style) {
    if (state.disabled) {
        return style.disabledBackground.a > 0.01f ? style.disabledBackground : style.normalBackground;
    }
    if (state.selected && style.selectedBackground.a > 0.01f) {
        return Color::Lerp(style.normalBackground, style.selectedBackground, 0.55f);
    }
    if (state.pressAnim > 0.01f && style.pressedBackground.a > 0.01f) {
        return Color::Lerp(style.normalBackground, style.pressedBackground, state.pressAnim);
    }
    if (state.hoverAnim > 0.01f && style.hoverBackground.a > 0.01f) {
        return Color::Lerp(style.normalBackground, style.hoverBackground, state.hoverAnim);
    }
    return style.normalBackground;
}

ResolvedControlBorder ResolveControlBorder(
    const InteractionState& state,
    ControlBorderMode mode,
    const Color& styleBorder = Color::Transparent()) {
    (void)mode;
    (void)styleBorder;
    if (state.focused) {
        const float width = ResolveMetric(MetricToken::FocusRingWidth);
        return { ResolveColor(ColorToken::BorderFocus), width };
    }
    return { Color::Transparent(), 0.0f };
}

void PaintControlFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    const ControlFrameStyle& style,
    ControlBorderMode borderMode,
    const Color& styleBorder = Color::Transparent()) {
    const Color bg = ResolveControlBackground(state, style);
    context.DrawRoundedRect(rect, bg, style.cornerRadius);

    const ResolvedControlBorder border = ResolveControlBorder(state, borderMode, styleBorder);
    if (border.color.a > 0.01f && border.width > 0.0f) {
        context.DrawRoundedRectOutline(rect, border.color, border.width, style.cornerRadius);
    }
}

float EdgeWidthPx() {
    return std::max(1.0f, ResolveMetric(MetricToken::BorderWidth));
}

} // namespace

void PaintSubtleDropShadow(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    Color shadow = ResolveColor(ColorToken::ShadowSubtle);
    shadow.a *= 0.22f * strength;
    const float blur = ResolveMetric(MetricToken::ShadowBlurSmall) * 0.65f;
    Rect shadowRect = rect;
    shadowRect.y += EdgeWidthPx();
    context.DrawShadow(shadowRect, shadow, radius, blur);
}

void PaintRaisedBevel(PaintContext& context, const Rect& rect, float radius, float strength) {
    (void)context;
    (void)rect;
    (void)radius;
    (void)strength;
}

void PaintSlateButtonBevel(PaintContext& context, const Rect& rect, float strength) {
    (void)context;
    (void)rect;
    (void)strength;
}

void PaintInsetBevel(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    const float w = EdgeWidthPx();
    const float inset = std::max(1.0f, w);
    const float edgeTrim = radius * 0.35f;

    Color innerShadow = ResolveColor(ColorToken::ShadowSubtle);
    innerShadow.a *= strength * 0.95f;
    Color innerHighlight = ResolveColor(ColorToken::HighlightSubtle);
    innerHighlight.a *= strength * 1.4f;

    context.DrawRect(
        Rect{ rect.x + edgeTrim, rect.y + inset, rect.width - edgeTrim * 2.0f, w },
        innerShadow);
    context.DrawRect(
        Rect{ rect.x + edgeTrim, rect.y + rect.height - inset - w, rect.width - edgeTrim * 2.0f, w },
        innerHighlight);
}

void PaintSubtleBorderDepth(PaintContext& context, const Rect& rect, float radius, float strength) {
    if (strength <= 0.01f) {
        return;
    }
    const float w = EdgeWidthPx();
    const float trim = radius * 0.25f;

    Color highlight = ResolveColor(ColorToken::HighlightSubtle);
    highlight.a *= strength * 1.35f;
    Color shade = ResolveColor(ColorToken::ShadowSubtle);
    shade.a *= strength * 0.32f;
    Color border = ResolveColor(ColorToken::BorderSubtle);
    border.a *= strength * 0.9f;

    context.DrawRect(
        Rect{ rect.x + trim, rect.y, rect.width - trim * 2.0f, w },
        highlight);
    context.DrawRect(
        Rect{ rect.x + trim, rect.y + rect.height - w, rect.width - trim * 2.0f, w },
        shade);
    context.DrawRoundedRectOutline(rect, border, w, radius);
}

void PaintPanelButtonFace(
    PaintContext& context,
    const Rect& rect,
    const Color& background,
    float radius,
    float hoverAnim,
    float pressAnim,
    bool emphasized) {
    (void)hoverAnim;
    (void)pressAnim;
    (void)emphasized;
    context.DrawRoundedRect(rect, background, radius);
}

namespace {

ControlFrameStyle InputFrameStyle() {
    const ResolvedStyle base = Role(StyleRole::Input);
    ControlFrameStyle style;
    style.normalBackground = base.background;
    style.hoverBackground = ResolveColor(ColorToken::ControlBackgroundHover);
    style.disabledBackground = ResolveColor(ColorToken::DisabledBackground);
    style.cornerRadius = base.cornerRadius > 0.0f
        ? base.cornerRadius
        : ResolveMetric(MetricToken::CornerRadiusSmall);
    return style;
}

float SearchInputCornerRadius(const Rect& rect) {
    return rect.height * 0.5f;
}

void PaintInputFrameInternal(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    float cornerRadius) {
    context.PushSurfaceOwner("Input", SurfaceRole::Input);

    SurfaceRole fillRole = SurfaceRole::Input;
    if (state.disabled) {
        fillRole = SurfaceRole::Disabled;
    } else if (state.pressAnim > 0.01f) {
        fillRole = SurfaceRole::ControlPressed;
    } else if (state.hoverAnim > 0.01f) {
        fillRole = SurfaceRole::ControlHover;
    }
    context.DrawSurface(rect, fillRole, cornerRadius, "Input");

    if (state.focused) {
        context.DrawRoundedRectOutline(
            rect,
            ResolveColor(ColorToken::BorderFocus),
            EdgeWidthPx(),
            cornerRadius);
    }
    context.PopSurfaceOwner();
}

} // namespace

ResolvedStyle Role(StyleRole role) {
    return ThemeManager::Get().Resolve(role);
}

float HoverDamping() {
    return ResolveMetric(MetricToken::HoverAnimationDamping);
}

float PressDamping() {
    return ResolveMetric(MetricToken::PressAnimationDamping);
}

void PaintElevation(PaintContext& context, const Rect& rect, int elevation, float radius) {
    if (elevation <= 0) {
        return;
    }
    Color shadow = ResolveColor(elevation >= 3 ? ColorToken::ShadowOverlay
        : elevation >= 2 ? ColorToken::ShadowPopup
        : ColorToken::ShadowSubtle);
    const float blur = elevation >= 2
        ? ResolveMetric(MetricToken::ShadowBlurMedium)
        : ResolveMetric(MetricToken::ShadowBlurSmall);
    context.DrawShadow(rect, shadow, radius, blur);
}

void PaintPopupShadow(PaintContext& context, const Rect& rect, float radius) {
    PaintElevation(context, rect, 2, radius);
}

void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float cornerRadius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    SurfaceRole surfaceRole)
{
    const Color fill = ResolveInteractiveSurfaceColor(surfaceRole, hoverAnim, pressAnim, selected);
    if (fill.a > 0.001f) {
        SurfaceRole drawRole = surfaceRole;
        if (selected) {
            drawRole = SurfaceRole::Selected;
        } else if (pressAnim > 0.01f) {
            drawRole = SurfaceRole::ControlPressed;
        } else if (hoverAnim > 0.01f) {
            drawRole = SurfaceRole::ControlHover;
        }
        context.DrawSurface(rect, drawRole, cornerRadius, "InteractiveFill");
    }
}

void PaintInteractiveFill(
    PaintContext& context,
    const Rect& rect,
    float cornerRadius,
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    PaintInteractiveFill(
        context,
        rect,
        cornerRadius,
        hoverAnim,
        pressAnim,
        selected,
        SurfaceRoleFromColorToken(surfaceToken));
}

void PaintFocusRing(PaintContext& context, const Rect& rect, float radius) {
    context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderFocus), 1.0f, radius);
}

void PaintFilledButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state,
    StyleRole hoverRole,
    StyleRole pressRole) {
    if (state.disabled) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::DisabledBackground), base.cornerRadius);
        return;
    }

    if (base.elevation <= 0) {
        // Flat chrome — no drop shadow; surface color only.
    } else {
        PaintElevation(context, rect, base.elevation, base.cornerRadius);
    }

    const float bevelStrength = std::max(0.0f, 1.0f - state.pressAnim * 0.8f);
    ControlFrameStyle frame;
    frame.normalBackground = base.background;
    frame.hoverBackground = Role(hoverRole).background;
    frame.pressedBackground = Role(pressRole).background;
    frame.selectedBackground = ResolveColor(ColorToken::SelectedBackground);
    frame.cornerRadius = base.cornerRadius;
    PaintControlFrame(
        context,
        rect,
        state,
        frame,
        base.border.a > 0.01f ? ControlBorderMode::Subtle : ControlBorderMode::Styled,
        base.border);
    PaintRaisedBevel(context, rect, frame.cornerRadius, bevelStrength);
}

void PaintGhostButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state) {
    InteractionState local = state;
    ResolvedStyle ghost = base;
    ghost.background = Color::Transparent();
    ghost.border = Color::Transparent();
    if (local.hoverAnim > 0.01f || local.pressAnim > 0.01f || local.selected) {
        PaintFilledButton(context, rect, Role(StyleRole::ButtonHover), local);
    } else {
        PaintFilledButton(context, rect, ghost, local);
    }
}

void PaintIconButtonFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state,
    bool active) {
    ResolvedStyle base = Role(active ? StyleRole::IconButtonPressed : StyleRole::IconButton);
    if (state.hoverAnim > 0.01f && !active) {
        base = Role(StyleRole::IconButtonHover);
    }
    PaintFilledButton(context, rect, base, state, StyleRole::IconButtonHover, StyleRole::IconButtonPressed);
}

void PaintBorderlessIconButton(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    const float interaction = std::max(state.hoverAnim, state.pressAnim);
    if (interaction > 0.01f) {
        Color bg = state.pressAnim > state.hoverAnim
            ? ResolveColor(ColorToken::PressedBackground)
            : ResolveColor(ColorToken::HoverBackground);
        bg.a *= interaction;
        const float radius = ResolveMetric(MetricToken::IconButtonRadius);
        context.DrawRoundedRect(rect, bg, radius);
    }
}

void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    PaintInputFrameInternal(context, rect, state, InputFrameStyle().cornerRadius);
}

void PaintStatusBarCommandField(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    // UE footer console: always-recessed well that blends into the status strip.
    Color fill = ResolveColor(ColorToken::InputBackground);
    if (state.hoverAnim > 0.01f && !state.focused) {
        fill = Color::Lerp(fill, ResolveColor(ColorToken::SecondarySurface), state.hoverAnim * 0.28f);
    }
    context.DrawRect(rect, fill);

    if (state.focused) {
        context.DrawRoundedRectOutline(
            rect,
            ResolveColor(ColorToken::BorderSubtle),
            EdgeWidthPx(),
            0.0f);
    }
}

void PaintSearchInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    const float radius = SearchInputCornerRadius(rect);
    context.DrawRoundedRect(rect, ResolveColor(ColorToken::InputBackground), radius);

    if (state.focused) {
        Color border = ResolveColor(ColorToken::BorderSubtle);
        context.DrawRoundedRectOutline(rect, border, EdgeWidthPx(), radius);
    }
}

void PaintSearchField(
    PaintContext& context,
    const Rect& rect,
    const std::string& placeholder,
    const std::string& text,
    const InteractionState& state,
    bool showCaret,
    const SearchFieldPaintOptions& options) {
    if (!options.toolbarFlat) {
        PaintSearchInputFrame(context, rect, state);
    }

    const float padH = options.toolbarFlat
        ? LayoutMetrics::SearchInputPaddingH() * 0.5f
        : LayoutMetrics::SearchInputPaddingH();
    const float iconPx = LayoutMetrics::SearchInputIconSize();
    const float fontSize = LayoutMetrics::SearchInputFontSize();
    const float iconGap = ResolveMetric(MetricToken::Space1);

    const float iconX = rect.x + padH;
    Rect iconBand{ iconX, rect.y, iconPx, rect.height };
    IconPainter::Draw(
        context,
        WindIcons::Search16,
        iconBand,
        static_cast<uint32_t>(iconPx));

    const float textX = iconX + iconPx + iconGap;
    const float textY = rect.y + (rect.height - fontSize) * 0.5f;
    const bool empty = text.empty();

    if (empty) {
        context.DrawText(
            placeholder,
            Point{ textX, textY },
            ResolveColor(ColorToken::SearchPlaceholder),
            fontSize);
    } else {
        context.DrawText(text, Point{ textX, textY }, ResolveColor(ColorToken::TextPrimary), fontSize);
        if (state.focused && showCaret) {
            const float caretX = textX + context.GetTextWidth(text, fontSize);
            context.DrawRect(
                Rect{ caretX, textY, ResolveMetric(MetricToken::BorderWidth), fontSize },
                ResolveColor(ColorToken::TextPrimary));
        }
    }

    if (options.showClearButton && !empty) {
        const float clearSize = iconPx;
        const float clearX = rect.x + rect.width - clearSize - padH;
        Rect clearBand{ clearX, rect.y, clearSize, rect.height };
        if (options.clearHovered) {
            const float radius = ResolveMetric(MetricToken::IconButtonRadius);
            context.DrawRoundedRect(
                Rect{ clearX, rect.y + (rect.height - clearSize) * 0.5f, clearSize, clearSize },
                ResolveColor(ColorToken::HoverBackground),
                radius);
        }
        IconPainter::Draw(
            context,
            WindIcons::Close16,
            clearBand,
            static_cast<uint32_t>(clearSize));
    }
}

void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    ResolvedStyle base = Role(StyleRole::TableRow);
    we::runtime::kindui::ControlState controlState = ControlState::Normal;
    if (state.disabled) {
        controlState = ControlState::Disabled;
    } else if (state.selected) {
        controlState = state.focused ? ControlState::Selected : ControlState::SelectedInactive;
        base = Role(StyleRole::TableRowSelected);
    } else if (state.hoverAnim > 0.01f) {
        controlState = ControlState::Hover;
        base = Role(StyleRole::TableRowHover);
    }

    const Color bg = ResolveControlColor(ControlKind::TreeRow, controlState);
    if (bg.a > 0.001f) {
        SurfaceRole role = SurfaceRole::Transparent;
        switch (controlState) {
        case ControlState::Selected: role = SurfaceRole::Selected; break;
        case ControlState::SelectedInactive: role = SurfaceRole::SelectedInactive; break;
        case ControlState::Hover: role = SurfaceRole::ControlHover; break;
        case ControlState::Disabled: role = SurfaceRole::Disabled; break;
        default: break;
        }
        if (role != SurfaceRole::Transparent) {
            context.DrawSurface(rect, role, base.cornerRadius, "ListRow");
        }
    }

    const ResolvedControlBorder border = ResolveControlBorder(
        state,
        state.selected ? ControlBorderMode::Subtle : ControlBorderMode::None,
        base.border);
    if (border.color.a > 0.01f && border.width > 0.0f) {
        context.DrawRoundedRectOutline(rect, border.color, border.width, base.cornerRadius);
    }
}

void PaintCard(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    ResolvedStyle base = state.hoverAnim > 0.5f ? Role(StyleRole::CardHover) : Role(StyleRole::Card);
    if (state.hoverAnim > 0.01f && state.hoverAnim <= 0.5f) {
        base.background = Color::Lerp(
            Role(StyleRole::Card).background,
            Role(StyleRole::CardHover).background,
            state.hoverAnim);
        base.border = Color::Lerp(
            Role(StyleRole::Card).border,
            Role(StyleRole::CardHover).border,
            state.hoverAnim);
    }
    // Cards elevate via surface luminance; shadow only on stronger hover.
    const int elevation = state.hoverAnim > 0.35f ? base.elevation : 0;
    PaintElevation(context, rect, elevation, base.cornerRadius);
    context.DrawRoundedRect(rect, base.background, base.cornerRadius);
}

void PaintCenteredLabel(
    PaintContext& context,
    const Rect& rect,
    const std::string& text,
    const Color& color,
    float fontSize,
    bool bold) {
    const float textW = context.GetTextWidth(text, fontSize, bold);
    context.DrawText(
        text,
        Point{
            rect.x + (rect.width - textW) * 0.5f,
            rect.y + (rect.height - fontSize) * 0.5f
        },
        color,
        fontSize,
        bold);
}

void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    const std::string& title,
    const std::string& subtitle) {
    const TypographySpec titleSpec = ResolveTypography(TypographyToken::SectionTitle);
    const TypographySpec subtitleSpec = ResolveTypography(TypographyToken::Subtitle);
    float y = rect.y;
    context.DrawText(
        title,
        Point{ rect.x, y },
        titleSpec.color,
        titleSpec.sizePx,
        titleSpec.bold);
    if (!subtitle.empty()) {
        y += titleSpec.lineHeightPx;
        context.DrawText(
            subtitle,
            Point{ rect.x, y },
            subtitleSpec.color,
            subtitleSpec.sizePx,
            subtitleSpec.bold);
    }
}

void PaintDangerButton(
    PaintContext& context,
    const Rect& rect,
    const ResolvedStyle& base,
    const InteractionState& state) {
    if (state.disabled) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::DisabledBackground), base.cornerRadius);
        return;
    }

    PaintElevation(context, rect, base.elevation, base.cornerRadius);

    ControlFrameStyle frame;
    frame.normalBackground = base.background;
    frame.hoverBackground = ResolveColor(ColorToken::ButtonDangerHover);
    frame.pressedBackground = ResolveColor(ColorToken::ButtonDangerPressed);
    frame.cornerRadius = base.cornerRadius;
    const float bevelStrength = std::max(0.0f, 1.0f - state.pressAnim * 0.8f);
    PaintControlFrame(context, rect, state, frame, ControlBorderMode::Styled, base.border);
    PaintRaisedBevel(context, rect, frame.cornerRadius, bevelStrength);
}

void PaintPopupSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Popup);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
}

void PaintTooltipSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Tooltip);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
}

void PaintCheckbox(
    PaintContext& context,
    const Rect& box,
    bool checked,
    const InteractionState& state) {
    const ResolvedStyle style = Role(StyleRole::Checkbox);
    Color bg = style.background;
    if (state.hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), state.hoverAnim);
    }
    if (checked) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::AccentPrimary), 0.85f);
    }
    context.DrawRoundedRect(box, bg, style.cornerRadius);
    if (checked) {
        const float inset = std::max(2.0f, box.width * 0.22f);
        context.DrawRoundedRect(
            Rect{ box.x + inset, box.y + inset, box.width - inset * 2.0f, box.height - inset * 2.0f },
            ResolveColor(ColorToken::TextPrimary),
            std::max(1.0f, style.cornerRadius * 0.5f));
    }
}

void PaintPanelTab(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    const InteractionState& state) {
    const float radius = 0.0f;
    if (state.selected) {
        context.DrawRoundedRect(
            bounds,
            ResolveSurfaceColor(SurfaceRole::TabActive),
            radius);
    } else if (state.hoverAnim > 0.01f || state.pressAnim > 0.01f) {
        Color tabBg = Color::Lerp(
            Color::Transparent(),
            ResolveColor(ColorToken::HoverBackground),
            std::max(state.hoverAnim, state.pressAnim));
        if (tabBg.a > 0.01f) {
            PaintPanelButtonFace(
                context,
                bounds,
                tabBg,
                radius,
                state.hoverAnim,
                state.pressAnim,
                false);
        }
    }

    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption);
    const Color textColor = state.selected
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
    context.DrawText(
        std::string(label),
        Point{
            bounds.x + ResolveMetric(MetricToken::Space3),
            bounds.y + (bounds.height - fontSize) * 0.5f
        },
        textColor,
        fontSize,
        false);
}

void PaintVerticalSeparator(
    PaintContext& context,
    float x,
    float top,
    float bottom,
    float thickness,
    ColorToken colorToken)
{
    const float height = bottom - top;
    if (height <= 0.0f) {
        return;
    }

    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    const float lineWidth = (std::max)(1.0f, IconMetrics::SnapPx(
        (thickness > 0.0f ? thickness : ResolveMetric(MetricToken::BorderWidth)) * uiScale));
    const float snappedX = IconMetrics::SnapPx(x - lineWidth * 0.5f);
    context.DrawRect(Rect{ snappedX, top, lineWidth, height }, ResolveColor(colorToken));
}

} // namespace ControlChrome
} // namespace we::runtime::kindui
