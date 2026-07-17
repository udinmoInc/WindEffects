#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeColors.h"
#include "KindUI/Tokens/DesignToken.h"


namespace we::runtime::kindui::ToolbarButtonChrome {
namespace {

Color MakePressBackground(float strength) {
    Color pressed = ResolveColor(ColorToken::PressedBackground);
    pressed.a *= strength;
    return pressed;
}

} // namespace

float ButtonRadius(float uiScale) {
    return ResolveMetric(MetricToken::IconButtonRadius) * uiScale;
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

Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx) {
    return IconMetrics::PlaceGlyphCentered(controlBounds, glyphTierPx);
}

Color ResolveIconColor(float hoverAnim, float pressStrength, bool active) {
    return we::runtime::kindui::ResolveIconColor(
        IconColorRole::Primary,
        hoverAnim,
        pressStrength,
        active);
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
    const Color selectedHoverBg = Color::Lerp(selectedBg, hoverBg, 0.55f);

    if (active || activeAnim > 0.01f) {
        Color bg = selectedBg;
        if (hoverAnim > 0.01f || pressStrength > 0.01f) {
            bg = Color::Lerp(selectedBg, selectedHoverBg, std::max(hoverAnim, pressStrength));
        }
        bg.a *= active ? 1.0f : activeAnim;
        context.DrawRoundedRect(rect, bg, radius);
    } else if (pressStrength > 0.01f) {
        context.DrawRoundedRect(rect, MakePressBackground(pressStrength), radius);
    } else if (hoverAnim > 0.01f) {
        Color bg = Color::Lerp(Color{0.0f, 0.0f, 0.0f, 0.0f}, hoverBg, hoverAnim);
        context.DrawRoundedRect(rect, bg, radius);
    }
}

void PaintChipDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    Color idleBg = ResolveColor(ColorToken::ButtonPrimaryBackground);
    idleBg.a = 0.38f;
    const Color hoverBg = ResolveColor(ColorToken::ButtonPrimaryHover);
    const Color pressBg = ResolveColor(ColorToken::ButtonPrimaryPressed);

    Color bg = idleBg;
    if (pressStrength > 0.01f) {
        bg = Color::Lerp(hoverBg, pressBg, pressStrength);
        bg.a = 1.0f;
    } else {
        bg = Color::Lerp(idleBg, hoverBg, hoverAnim);
    }
    context.DrawRoundedRect(rect, bg, radius);

    const float emphasis = std::max(hoverAnim, pressStrength);
    if (emphasis > 0.01f) {
        Color border = ResolveColor(ColorToken::BorderLight);
        border.a *= emphasis * 0.65f;
        context.DrawRoundedRectOutline(rect, border, 1.0f * uiScale, radius);
    }
}

void PaintViewportChip(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;
    Color idleBg = ResolveColor(ColorToken::ViewportToolbarBackground);
    Color hoverBg = Color::Lerp(idleBg, ResolveColor(ColorToken::ButtonPrimaryHover), 0.55f);
    Color pressBg = ResolveColor(ColorToken::ButtonPrimaryPressed);

    context.DrawShadow(
        rect,
        ResolveColor(ColorToken::ShadowSubtle),
        radius,
        ResolveMetric(MetricToken::Space2) * uiScale);

    Color bg = idleBg;
    if (pressStrength > 0.01f) {
        bg = Color::Lerp(hoverBg, pressBg, pressStrength);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(idleBg, hoverBg, hoverAnim);
    }
    context.DrawRoundedRect(rect, bg, radius);
    context.DrawRoundedRectOutline(
        rect,
        ResolveColor(ColorToken::BorderDefault),
        1.0f * uiScale,
        radius);

    const float emphasis = std::max(hoverAnim, pressStrength);
    if (emphasis > 0.01f) {
        Color border = ResolveColor(ColorToken::BorderLight);
        border.a *= emphasis * 0.75f;
        context.DrawRoundedRectOutline(rect, border, 1.0f * uiScale, radius);
    }
}

} // namespace we::runtime::kindui::ToolbarButtonChrome
