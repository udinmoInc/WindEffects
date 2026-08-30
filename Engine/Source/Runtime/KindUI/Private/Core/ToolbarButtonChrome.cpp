#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui::ToolbarButtonChrome {

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
    const float interaction = std::max({hoverAnim, pressStrength, active ? 1.0f : activeAnim});

    if (active || activeAnim > 0.01f) {
        PaintActiveIndicator(context, rect, active ? 1.0f : activeAnim, uiScale);
    }

    if (interaction > 0.01f) {
        ControlChrome::PaintInteractiveFill(
            context,
            rect,
            radius,
            hoverAnim,
            pressStrength,
            active,
            ColorToken::PanelBackground);
        const float bevelStrength = std::max(0.0f, interaction * (1.0f - pressStrength * 0.75f));
        ControlChrome::PaintRaisedBevel(context, rect, radius, bevelStrength);
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
    const float inset = HorizontalPad(uiScale) * 1.5f;
    context.DrawRect(
        Rect{
            rect.x + inset,
            rect.y + rect.height - thickness - 1.0f * uiScale,
            std::max(0.0f, rect.width - inset * 2.0f),
            thickness
        },
        ResolveColor(ColorToken::AccentPrimary));
}

void PaintInlineDropdown(
    PaintContext& context,
    const Rect& rect,
    float hoverAnim,
    float pressStrength,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    ControlChrome::PaintInteractiveFill(
        context,
        rect,
        radius,
        hoverAnim,
        pressStrength,
        false,
        ColorToken::PanelBackground);
}

void PaintExecutionCluster(
    PaintContext& context,
    const Rect& rect,
    float uiScale)
{
    const float radius = ButtonRadius(uiScale);
    context.DrawRoundedRectOutline(
        rect,
        ResolveColor(ColorToken::BorderDefault),
        std::max(1.0f, ResolveMetric(MetricToken::BorderWidth) * uiScale),
        radius);
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
    const Color idle = ResolveColor(ColorToken::ViewportToolbarBackground);
    const Color fill = (hoverAnim > 0.001f || pressStrength > 0.001f)
        ? ResolveInteractiveBackground(hoverAnim, pressStrength, false, ColorToken::ViewportToolbarBackground)
        : idle;
    context.DrawRoundedRect(rect, fill.a > 0.001f ? fill : idle, radius);
}

} // namespace we::runtime::kindui::ToolbarButtonChrome
