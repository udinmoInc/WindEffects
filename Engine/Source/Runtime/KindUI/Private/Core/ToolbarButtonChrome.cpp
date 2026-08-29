#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui::ToolbarButtonChrome {
namespace {

Color MakePressBackground(float strength) {
    Color pressed = ResolveColor(ColorToken::PressedBackground);
    pressed.a *= strength;
    return pressed;
}

} // namespace

float ButtonRadius(float uiScale) {
    return ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;
}

float IconSize(float uiScale) {
    (void)uiScale;
    return static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeToolbar));
}

float PrimaryIconSize(float uiScale) {
    (void)uiScale;
    return static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizePrimary));
}

float HorizontalPad(float uiScale) {
    return ResolveMetric(MetricToken::Space1) * uiScale;
}

float ChipHorizontalPad(float uiScale) {
    return ResolveMetric(MetricToken::Space2) * uiScale;
}

float IconGapPx(float uiScale) {
    return ResolveMetric(MetricToken::Space1) * uiScale;
}

float ChevronGapPx(float uiScale) {
    return ResolveMetric(MetricToken::Space2) * uiScale;
}

float ItemSize(float uiScale) {
    return ResolveMetric(MetricToken::IconButtonSize) * uiScale;
}

float RowContentHeight(float uiScale) {
    const float toolbarH = ResolveMetric(MetricToken::ToolbarHeight) * uiScale;
    const float itemH = ItemSize(uiScale);
    return std::min(toolbarH, std::max(itemH, ResolveMetric(MetricToken::HeaderControlHeight) * uiScale));
}

float GroupGap(float uiScale) {
    return ResolveMetric(MetricToken::ButtonGroupSpacing) * uiScale;
}

float ItemGap(float uiScale) {
    return ResolveMetric(MetricToken::ButtonSpacing) * uiScale;
}

Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx) {
    return IconMetrics::PlaceGlyphCentered(controlBounds, glyphTierPx);
}

Color ResolveIconColor(float hoverAnim, float pressStrength, bool active) {
    if (active) {
        return ResolveColor(ColorToken::IconAccent);
    }
    return we::runtime::kindui::ResolveIconColor(
        IconColorRole::Primary,
        hoverAnim,
        pressStrength,
        active);
}

Color ResolvePlayIconColor(float hoverAnim, float pressStrength, bool active) {
    Color play = ResolveColor(ColorToken::Success);
    if (active || pressStrength > 0.01f) {
        return Color::Lerp(play, ResolveColor(ColorToken::TextPrimary), std::min(1.0f, pressStrength * 0.35f));
    }
    if (hoverAnim > 0.01f) {
        return Color::Lerp(play, ResolveColor(ColorToken::TextPrimary), hoverAnim * 0.2f);
    }
    return play;
}

void PaintIconButton(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    const Color hoverBg = ResolveColor(ColorToken::HoverBackground);
    const Color selectedBg = ResolveColor(ColorToken::SelectedBackground);
    const Color pressBg = ResolveColor(ColorToken::PressedBackground);

    const float emphasis = std::max({ active ? 1.0f : activeAnim, hoverAnim, pressStrength });

    if (active || activeAnim > 0.01f) {
        Color bg = selectedBg;
        if (hoverAnim > 0.01f || pressStrength > 0.01f) {
            bg = Color::Lerp(selectedBg, hoverBg, std::max(hoverAnim, pressStrength) * 0.45f);
        }
        bg.a *= active ? 1.0f : activeAnim;
        context.DrawRoundedRect(rect, bg, radius);
        PaintActiveIndicator(context, rect, active ? 1.0f : activeAnim, uiScale);
    } else if (pressStrength > 0.01f) {
        Color bg = Color::Lerp(Color::Transparent(), pressBg, pressStrength);
        context.DrawRoundedRect(rect, bg, radius);
    } else if (hoverAnim > 0.01f) {
        Color bg = Color::Lerp(Color::Transparent(), hoverBg, hoverAnim);
        context.DrawRoundedRect(rect, bg, radius);
    }
}

void PaintActiveIndicator(
    PaintContext& context,
    const Rect& rect,
    float activeAnim,
    float uiScale)
{
    if (activeAnim <= 0.01f) {
        return;
    }
    const float thickness = std::max(1.0f, ResolveMetric(MetricToken::BorderWidth) * uiScale);
    const float inset = HorizontalPad(uiScale);
    Color accent = ResolveColor(ColorToken::ActiveTabLine);
    accent.a *= activeAnim;
    context.DrawRect(
        Rect{
            rect.x + inset,
            rect.y + rect.height - thickness,
            std::max(0.0f, rect.width - inset * 2.0f),
            thickness
        },
        accent);
}

void PaintInlineDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    const Color hoverBg = ResolveColor(ColorToken::HoverBackground);
    const Color pressBg = ResolveColor(ColorToken::PressedBackground);

    if (pressStrength > 0.01f) {
        Color bg = Color::Lerp(Color::Transparent(), pressBg, pressStrength);
        context.DrawRoundedRect(rect, bg, radius);
    } else if (hoverAnim > 0.01f) {
        Color bg = Color::Lerp(Color::Transparent(), hoverBg, hoverAnim);
        context.DrawRoundedRect(rect, bg, radius);
    }
}

void PaintExecutionCluster(
    PaintContext& context,
    const Rect& rect,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    Color bg = ResolveColor(ColorToken::SelectedBackground);
    bg.a = 0.55f;
    context.DrawRoundedRect(rect, bg, radius);
}

void PaintChipDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    PaintInlineDropdown(context, rect, hoverAnim, pressStrength, uiScale);
}

void PaintViewportChip(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    Color idleBg = ResolveColor(ColorToken::ViewportToolbarBackground);
    Color hoverBg = Color::Lerp(idleBg, ResolveColor(ColorToken::HoverBackground), 0.65f);
    Color pressBg = ResolveColor(ColorToken::PressedBackground);

    Color bg = idleBg;
    if (pressStrength > 0.01f) {
        bg = Color::Lerp(hoverBg, pressBg, pressStrength);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(idleBg, hoverBg, hoverAnim);
    }
    context.DrawRoundedRect(rect, bg, radius);
}

} // namespace we::runtime::kindui::ToolbarButtonChrome
