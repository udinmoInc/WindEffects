#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Tokens/DesignToken.h"

#include "KindUI/Theming/PaletteRuntime.h"

#include <algorithm>
#include <string_view>

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
    // Grayscale multiply: muted at rest → luminous on hover. Preserves authored icon hue.
    float brightness = active ? 1.06f : 0.78f;
    brightness = brightness + (1.12f - brightness) * std::clamp(hoverAnim, 0.0f, 1.0f);
    if (pressStrength > 0.001f) {
        brightness = brightness + (1.00f - brightness) * std::clamp(pressStrength, 0.0f, 1.0f) * 0.35f;
    }
    return Color{ brightness, brightness, brightness, 1.0f };
}

Color ResolvePlayIconColor(float hoverAnim, float pressStrength, bool active) {
    // Same lighting model — play glyph keeps its authored green.
    return ResolveIconColor(hoverAnim, pressStrength, active);
}

[[nodiscard]] bool IsAuthoredColorIcon(WindIconRef icon) {
    if (!icon.IsValid() || icon.stem == nullptr) {
        return false;
    }
    const std::string_view stem(icon.stem);
    return stem == "folder"
        || stem == "folder-open"
        || stem == "folder-mask"
        || stem == "folder-open-mask"
        || stem == "content-folder"
        || stem == "folder-create";
}

void PaintFloatingIcon(
    PaintContext& context,
    WindIconRef icon,
    const Rect& controlBounds,
    float glyphPx,
    float hoverAnim,
    float pressStrength,
    bool active)
{
    if (!icon.IsValid() || controlBounds.width < 1.0f || controlBounds.height < 1.0f) {
        return;
    }

    // Authored-color icons (e.g. folder): never recolor — only optional soft glow on hover.
    const bool preserveColor = IsAuthoredColorIcon(icon);
    const float hover = std::clamp(hoverAnim, 0.0f, 1.0f);
    const float light = std::max(hover, active ? 0.55f : 0.0f);

    if (light > 0.01f) {
        const float glowPx = glyphPx + 2.0f;
        Color glow{ 1.18f, 1.18f, 1.18f, (preserveColor ? 0.10f : 0.14f) * light };
        IconPainter::Draw(
            context,
            icon,
            controlBounds,
            static_cast<uint32_t>(glowPx),
            glow);
    }

    const Color tint = preserveColor
        ? Color::White()
        : ResolveIconColor(hoverAnim, pressStrength, active);
    IconPainter::Draw(
        context,
        icon,
        controlBounds,
        static_cast<uint32_t>(glyphPx),
        tint);
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
    // Chip / labeled surfaces only — icon-only controls use PaintFloatingIcon instead.
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
    // Standalone floating icons: no background, border, pill, or hover box.
    (void)context;
    (void)rect;
    (void)hoverAnim;
    (void)pressStrength;
    (void)active;
    (void)activeAnim;
    (void)uiScale;
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
    // Viewport controls are floating icons/labels — no rounded hover/active fill.
    (void)context;
    (void)rect;
    (void)hoverAnim;
    (void)pressStrength;
    (void)uiScale;
}

} // namespace we::runtime::kindui::ToolbarButtonChrome
 
