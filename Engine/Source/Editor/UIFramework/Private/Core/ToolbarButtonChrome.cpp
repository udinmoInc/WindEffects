#include "Core/ToolbarButtonChrome.h"
#include "Rendering/IconMetrics.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

namespace WindEffects::Editor::UI::ToolbarButtonChrome {
namespace {

Color MakePressBackground(float strength) {
    Color pressed = ResolveThemeColor(ThemeToken::PressedBackground);
    pressed.a *= strength;
    return pressed;
}

} // namespace

float ButtonRadius(float uiScale) {
    return ResolveThemeMetric(ThemeToken::IconButtonRadius) * uiScale;
}

float IconSize(float uiScale) {
    (void)uiScale;
    return static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizeToolbar));
}

float PrimaryIconSize(float uiScale) {
    (void)uiScale;
    return static_cast<float>(IconMetrics::GlyphTierPx(ThemeToken::IconSizePrimary));
}

float HorizontalPad(float uiScale) {
    return ResolveThemeMetric(ThemeToken::Space1) * uiScale;
}

float ChipHorizontalPad(float uiScale) {
    return ResolveThemeMetric(ThemeToken::Space2) * uiScale;
}

float IconGapPx(float uiScale) {
    return ResolveThemeMetric(ThemeToken::Space1) * uiScale;
}

float ChevronGapPx(float uiScale) {
    return ResolveThemeMetric(ThemeToken::Space2) * uiScale;
}

Rect PlaceIconInControl(const Rect& controlBounds, float glyphTierPx) {
    return IconMetrics::PlaceGlyphCentered(controlBounds, glyphTierPx);
}

Color ResolveIconColor(float hoverAnim, float pressStrength, bool active) {
    if (active) {
        return ResolveThemeColor(ThemeToken::AccentPrimary);
    }

    Color iconColor = ResolveThemeColor(ThemeToken::IconDefault);
    if (hoverAnim > 0.01f || pressStrength > 0.01f) {
        iconColor = Color::Lerp(
            iconColor,
            ResolveThemeColor(ThemeToken::IconHover),
            std::max(hoverAnim, pressStrength));
    }
    return iconColor;
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
    const Color hoverBg = ResolveThemeColor(ThemeToken::HoverBackground);
    const Color selectedBg = ResolveThemeColor(ThemeToken::SelectedBackground);
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
    Color idleBg = ResolveThemeColor(ThemeToken::ButtonPrimaryBackground);
    idleBg.a = 0.38f;
    const Color hoverBg = ResolveThemeColor(ThemeToken::ButtonPrimaryHover);
    const Color pressBg = ResolveThemeColor(ThemeToken::ButtonPrimaryPressed);

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
        Color border = ResolveThemeColor(ThemeToken::BorderLight);
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
    const float radius = ResolveThemeMetric(ThemeToken::CornerRadiusSmall) * uiScale;
    Color idleBg = ResolveThemeColor(ThemeToken::ViewportToolbarBackground);
    Color hoverBg = Color::Lerp(idleBg, ResolveThemeColor(ThemeToken::ButtonPrimaryHover), 0.55f);
    Color pressBg = ResolveThemeColor(ThemeToken::ButtonPrimaryPressed);

    context.DrawShadow(
        rect,
        ResolveThemeColor(ThemeToken::ShadowSubtle),
        radius,
        ResolveThemeMetric(ThemeToken::Space2) * uiScale);

    Color bg = idleBg;
    if (pressStrength > 0.01f) {
        bg = Color::Lerp(hoverBg, pressBg, pressStrength);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(idleBg, hoverBg, hoverAnim);
    }
    context.DrawRoundedRect(rect, bg, radius);
    context.DrawRoundedRectOutline(
        rect,
        ResolveThemeColor(ThemeToken::BorderDefault),
        1.0f * uiScale,
        radius);

    const float emphasis = std::max(hoverAnim, pressStrength);
    if (emphasis > 0.01f) {
        Color border = ResolveThemeColor(ThemeToken::BorderLight);
        border.a *= emphasis * 0.75f;
        context.DrawRoundedRectOutline(rect, border, 1.0f * uiScale, radius);
    }
}

} // namespace WindEffects::Editor::UI::ToolbarButtonChrome
