#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Tokens/DesignToken.h"

#include "KindUI/Theming/PaletteRuntime.h"

#include <algorithm>

namespace we::runtime::kindui::ToolbarButtonChrome {

float ButtonRadius(float uiScale) {
    return ResolveMetric(MetricToken::CornerRadiusSmall) * uiScale;
}

float IconSize(float uiScale) {
    (void)uiScale;
    return ResolveMetric(MetricToken::IconSizeToolbar);
}

float PrimaryIconSize(float uiScale) {
    (void)uiScale;
    return ResolveMetric(MetricToken::IconSizeToolbar);
}

float NavigationIconSize(float uiScale) {
    (void)uiScale;
    return ResolveMetric(MetricToken::IconSizeNavigation);
}

float WindowControlIconSize(float uiScale) {
    (void)uiScale;
    return ResolveMetric(MetricToken::IconSizeWindowControl);
}

float HorizontalPad(float uiScale) {
    return ResolveMetric(MetricToken::ButtonPaddingHorizontal) * uiScale;
}

float ChipHorizontalPad(float uiScale) {
    return ResolveMetric(MetricToken::SpaceMD) * uiScale;
}

float IconGapPx(float uiScale) {
    return ResolveMetric(MetricToken::Space1) * uiScale;
}

float ChevronGapPx(float uiScale) {
    return ResolveMetric(MetricToken::SpaceMD) * uiScale;
}

float ItemSize(float uiScale) {
    return 26.0f * uiScale;
}

float RowContentHeight(float uiScale) {
    return 26.0f * uiScale;
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
        return ResolveColor(ColorToken::TextOnAccent);
    }
    return we::runtime::kindui::ResolveIconColor(
        IconColorRole::Primary,
        hoverAnim,
        pressStrength,
        active);
}

Color ResolvePlayIconColor(float hoverAnim, float pressStrength, bool active) {
    Color play = ResolveColor(ColorToken::Success);
    if (active) {
        return ResolveColor(ColorToken::TextPrimary);
    }
    Color hover = ResolveColor(ColorToken::TextPrimary);
    Color result = Color::Pick(play, hover, std::clamp(hoverAnim, 0.0f, 1.0f) * 0.35f);
    return Color::Pick(result, hover, std::clamp(pressStrength, 0.0f, 1.0f) * 0.45f);
}

namespace {

void PaintSubtleToolbarFill(
    PaintContext& context,
    const Rect& rect,
    float radius,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim)
{
    const Color borderColor = ResolveColor(ColorToken::BorderSubtle);
    if (active || activeAnim >= 0.5f) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::SelectedBackground), radius);
        return;
    }

    if (hoverAnim <= 0.001f && pressStrength <= 0.001f) {
        return;
    }

    Color bgHover = ResolveColor(ColorToken::HoverBackground);
    Color bgPress = ResolveColor(ColorToken::ControlBackgroundPressed);

    Color bgColor = bgHover;
    if (pressStrength > 0.001f) {
        bgColor = Color::Pick(bgColor, bgPress, std::clamp(pressStrength, 0.0f, 1.0f));
    }

    context.DrawRoundedRect(rect, bgColor, radius);
}

} // namespace

void PaintToolbarButtonSurface(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    bool active,
    float activeAnim,
    float uiScale)
{
    PaintSubtleToolbarFill(
        context,
        rect,
        ButtonRadius(uiScale),
        hoverAnim,
        pressStrength,
        active,
        activeAnim);
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
    PaintToolbarButtonSurface(context, rect, hoverAnim, pressStrength, active, activeAnim, uiScale);
}

void PaintActiveIndicator(
    PaintContext& context,
    const Rect& rect,
    float activeAnim,
    float uiScale)
{
    (void)context;
    (void)rect;
    (void)activeAnim;
    (void)uiScale;
}

void PaintInlineDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    PaintSubtleToolbarFill(
        context,
        rect,
        ButtonRadius(uiScale),
        hoverAnim,
        pressStrength,
        false,
        0.0f);
}

void PaintExecutionCluster(
    PaintContext& context,
    const Rect& rect,
    float uiScale)
{
    (void)context;
    (void)rect;
    (void)uiScale;
    // Variant without recessed card background fill
}

void PaintStatusBarControl(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    bool active,
    float uiScale)
{
    if (active) {
        const float thickness = std::max(1.0f, ResolveMetric(MetricToken::BorderWidth) * uiScale);
        context.DrawRect(
            Rect{
                rect.x + HorizontalPad(uiScale),
                rect.y + rect.height - thickness,
                std::max(0.0f, rect.width - HorizontalPad(uiScale) * 2.0f),
                thickness
            },
            ResolveColor(ColorToken::ActiveTabLine));
        return;
    }

    const Color hover = ResolveInteractiveBackground(
        hoverAnim, 0.0f, false, ColorToken::ViewportToolbarBackground);
    if (hover.a > 0.001f) {
        context.DrawRect(rect, hover);
    }
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
    PaintInlineDropdown(context, rect, hoverAnim, pressStrength, uiScale);
}

} // namespace we::runtime::kindui::ToolbarButtonChrome
 
