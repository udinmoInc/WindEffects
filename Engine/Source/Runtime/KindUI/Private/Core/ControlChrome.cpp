#include "KindUI/Core/ControlChrome.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <string>
#include <string_view>


namespace we::runtime::kindui {
namespace ControlChrome {

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
    // Prefer luminance steps + 1 px borders; keep shadows soft for floating layers only.
    Color shadow = ResolveColor(ColorToken::ShadowSubtle);
    shadow.a *= 0.65f;
    if (elevation >= 2) {
        shadow = ResolveColor(ColorToken::ShadowPopup);
    }
    if (elevation >= 3) {
        shadow = ResolveColor(ColorToken::ShadowOverlay);
    }
    const float blur = elevation >= 2
        ? ResolveMetric(MetricToken::ShadowBlurMedium)
        : ResolveMetric(MetricToken::ShadowBlurSmall);
    context.DrawShadow(rect, shadow, radius, blur);
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
        Color bg = base.background;
        bg.a *= 0.45f;
        context.DrawRoundedRect(rect, bg, base.cornerRadius);
        return;
    }

    Color bg = base.background;
    if (state.pressAnim > 0.01f) {
        bg = Color::Lerp(bg, Role(pressRole).background, state.pressAnim);
    } else if (state.hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, Role(hoverRole).background, state.hoverAnim);
    }
    if (state.selected) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::SelectedBackground), 0.55f);
    }

    PaintElevation(context, rect, base.elevation, base.cornerRadius);
    context.DrawRoundedRect(rect, bg, base.cornerRadius);
    if (state.focused) {
        context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderFocus), 1.0f, base.cornerRadius);
    } else if (base.border.a > 0.01f) {
        Color border = base.border;
        if (state.hoverAnim > 0.01f) {
            border = Color::Lerp(border, ResolveColor(ColorToken::BorderLight), state.hoverAnim);
        }
        context.DrawRoundedRectOutline(rect, border, 1.0f, base.cornerRadius);
    }
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

void PaintInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    ResolvedStyle base = Role(StyleRole::Input);
    Color bg = base.background;
    if (state.disabled) {
        bg = ResolveColor(ColorToken::DisabledBackground);
    }
    const float radius = base.cornerRadius > 0.0f ? base.cornerRadius : ResolveMetric(MetricToken::CornerRadiusSmall);
    context.DrawRoundedRect(rect, bg, radius);
    Color border = state.focused
        ? ResolveColor(ColorToken::BorderFocus)
        : (state.hoverAnim > 0.01f
            ? Color::Lerp(ResolveColor(ColorToken::BorderDefault), ResolveColor(ColorToken::BorderLight), state.hoverAnim)
            : ResolveColor(ColorToken::BorderDefault));
    context.DrawRoundedRectOutline(rect, border, 1.0f, radius);
}

void PaintSearchInputFrame(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    Color bg = ResolveColor(ColorToken::InputBackground);
    if (state.disabled) {
        bg = ResolveColor(ColorToken::DisabledBackground);
    }
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall);
    context.DrawRoundedRect(rect, bg, radius);
    Color border = state.focused
        ? ResolveColor(ColorToken::BorderFocus)
        : (state.hoverAnim > 0.01f
            ? Color::Lerp(ResolveColor(ColorToken::BorderDefault), ResolveColor(ColorToken::BorderLight), state.hoverAnim)
            : ResolveColor(ColorToken::BorderDefault));
    context.DrawRoundedRectOutline(rect, border, 1.0f, radius);
}

void PaintListRow(
    PaintContext& context,
    const Rect& rect,
    const InteractionState& state) {
    ResolvedStyle base = Role(StyleRole::TableRow);
    if (state.selected) {
        base = Role(StyleRole::TableRowSelected);
    } else if (state.hoverAnim > 0.01f) {
        base = Role(StyleRole::TableRowHover);
        base.background = Color::Lerp(
            Color::Transparent(),
            Role(StyleRole::TableRowHover).background,
            state.hoverAnim);
    } else {
        base.background = Color::Transparent();
    }
    context.DrawRoundedRect(rect, base.background, base.cornerRadius);
    if (state.focused) {
        context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderFocus), 1.0f, base.cornerRadius);
    } else if (state.selected && base.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, base.border, 1.0f, base.cornerRadius);
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
    const Color border = base.border.a > 0.01f
        ? base.border
        : ResolveColor(ColorToken::BorderSubtle);
    const float borderWidth = base.borderWidth > 0.0f
        ? base.borderWidth
        : ResolveMetric(MetricToken::BorderWidth);
    context.DrawRoundedRectOutline(rect, border, borderWidth, base.cornerRadius);
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
        Color bg = base.background;
        bg.a *= 0.45f;
        context.DrawRoundedRect(rect, bg, base.cornerRadius);
        return;
    }

    Color bg = base.background;
    if (state.pressAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonDangerPressed), state.pressAnim);
    } else if (state.hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonDangerHover), state.hoverAnim);
    }

    PaintElevation(context, rect, base.elevation, base.cornerRadius);
    context.DrawRoundedRect(rect, bg, base.cornerRadius);
    if (state.focused) {
        context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderFocus), 1.0f, base.cornerRadius);
    } else if (base.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, base.border, 1.0f, base.cornerRadius);
    }
}

void PaintPopupSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Popup);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
    if (style.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, style.border, 1.0f, style.cornerRadius);
    }
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
    Color border = state.focused ? ResolveColor(ColorToken::BorderFocus) : style.border;
    context.DrawRoundedRectOutline(box, border, 1.0f, style.cornerRadius);
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
    if (state.selected) {
        const float inset = ResolveMetric(MetricToken::Space1);
        const float indicatorH = ResolveMetric(MetricToken::TabActiveIndicatorHeight);
        context.DrawRect(
            Rect{
                bounds.x + inset,
                bounds.y + bounds.height - indicatorH,
                bounds.width - inset * 2.0f,
                indicatorH
            },
            ResolveColor(ColorToken::AccentPrimary));
    } else if (state.hoverAnim > 0.01f) {
        context.DrawRect(
            bounds,
            Color::Lerp(
                Color::Transparent(),
                ResolveColor(ColorToken::HoverBackground),
                state.hoverAnim));
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
    float thickness)
{
    if (bottom <= top) {
        return;
    }
    const float scale = std::max(1.0f, DPIContext::GetScale());
    const float w = (std::max)(1.0f, thickness) * scale;
    context.DrawRect(
        Rect{ std::floor(x - w * 0.5f), top, w, bottom - top },
        ResolveColor(ColorToken::Separator));
}

} // namespace ControlChrome
} // namespace we::runtime::kindui
