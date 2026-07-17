#include "KindUI/Core/ControlChrome.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <string>

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
    Color shadow = ResolveColor(ColorToken::ShadowSubtle);
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
    const float width = ResolveMetric(MetricToken::FocusRingWidth);
    context.DrawRoundedRectOutline(rect, ResolveColor(ColorToken::BorderFocus), width, radius);
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
    if (base.border.a > 0.01f) {
        Color border = base.border;
        if (state.hoverAnim > 0.01f) {
            border = Color::Lerp(border, ResolveColor(ColorToken::BorderLight), state.hoverAnim);
        }
        context.DrawRoundedRectOutline(rect, border, base.borderWidth, base.cornerRadius);
    }
    if (state.focused) {
        PaintFocusRing(context, rect, base.cornerRadius);
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
    context.DrawRoundedRect(rect, bg, base.cornerRadius);
    Color border = state.focused
        ? ResolveColor(ColorToken::BorderFocus)
        : (state.hoverAnim > 0.01f
            ? Color::Lerp(ResolveColor(ColorToken::BorderDefault), ResolveColor(ColorToken::BorderLight), state.hoverAnim)
            : ResolveColor(ColorToken::BorderDefault));
    const float width = state.focused
        ? ResolveMetric(MetricToken::FocusRingWidth)
        : base.borderWidth;
    context.DrawRoundedRectOutline(rect, border, width, base.cornerRadius);
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
    if (state.selected && base.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, base.border, 1.5f, base.cornerRadius);
    }
    if (state.focused) {
        PaintFocusRing(context, rect, base.cornerRadius);
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
    }
    PaintElevation(context, rect, base.elevation, base.cornerRadius);
    context.DrawRoundedRect(rect, base.background, base.cornerRadius);
    context.DrawRoundedRectOutline(rect, base.border, base.borderWidth, base.cornerRadius);
}

void PaintCenteredLabel(
    PaintContext& context,
    const Rect& rect,
    const std::string& text,
    const Color& color,
    float fontSize,
    bool bold) {
    const float textW = static_cast<float>(text.size()) * fontSize * 0.55f;
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
    const ResolvedStyle header = Role(StyleRole::SectionHeader);
    const ResolvedStyle caption = Role(StyleRole::TextCaption);
    float y = rect.y + 4.0f;
    context.DrawText(title, Point{ rect.x, y }, header.foreground, header.fontSize, true);
    if (!subtitle.empty()) {
        y += header.fontSize + 4.0f;
        context.DrawText(subtitle, Point{ rect.x, y }, caption.foreground, caption.fontSize);
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
    if (base.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, base.border, base.borderWidth, base.cornerRadius);
    }
    if (state.focused) {
        PaintFocusRing(context, rect, base.cornerRadius);
    }
}

void PaintPopupSurface(PaintContext& context, const Rect& rect) {
    const ResolvedStyle style = Role(StyleRole::Popup);
    PaintElevation(context, rect, style.elevation > 0 ? style.elevation : 2, style.cornerRadius);
    context.DrawRoundedRect(rect, style.background, style.cornerRadius);
    if (style.border.a > 0.01f) {
        context.DrawRoundedRectOutline(rect, style.border, style.borderWidth, style.cornerRadius);
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
    context.DrawRoundedRectOutline(
        box,
        state.focused ? ResolveColor(ColorToken::BorderFocus) : style.border,
        state.focused ? ResolveMetric(MetricToken::FocusRingWidth) : style.borderWidth,
        style.cornerRadius);
    if (checked) {
        const float inset = std::max(2.0f, box.width * 0.22f);
        context.DrawRoundedRect(
            Rect{ box.x + inset, box.y + inset, box.width - inset * 2.0f, box.height - inset * 2.0f },
            ResolveColor(ColorToken::TextPrimary),
            std::max(1.0f, style.cornerRadius * 0.5f));
    }
}

} // namespace ControlChrome
} // namespace we::runtime::kindui
