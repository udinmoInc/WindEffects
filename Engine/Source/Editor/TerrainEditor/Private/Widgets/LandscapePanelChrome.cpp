#include "LandscapePanelChrome.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/Types.h"

#include <algorithm>

namespace we::editor::terrain::LandscapePanelChrome {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::ResolveColor;
using we::runtime::kindui::ResolveMetric;
namespace PanelChromeNs = we::editor::panels::PanelChrome;

float UiScale() {
    return std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
}

} // namespace

float PanelPad() { return PanelChromeNs::PanelPaddingH(); }
float SectionGap() { return ResolveMetric(MetricToken::Space2) * UiScale(); }
float RowHeight() { return 26.f * UiScale(); }
float ChipHeight() { return 30.f * UiScale(); }
float TabBarHeight() { return 32.f * UiScale(); }
float PrimaryButtonHeight() { return 34.f * UiScale(); }
float LabelColumnWidth() { return 108.f * UiScale(); }

void PaintPanelBackground(PaintContext& context, const Rect& bounds) {
    PanelChromeNs::PaintContentRegion(context, bounds);
}

void PaintTabBar(PaintContext& context, const Rect& bounds) {
    context.DrawRect(bounds, ResolveColor(ColorToken::PanelBackground));
    const float y = bounds.y + bounds.height - 1.f;
    context.DrawRect(Rect{bounds.x, y, bounds.width, 1.f}, ResolveColor(ColorToken::BorderSubtle));
}

void PaintTab(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    bool active,
    float hoverAnim)
{
    if (active) {
        context.DrawRect(bounds, ResolveColor(ColorToken::PanelTabActiveBackground));
        context.DrawRect(
            Rect{bounds.x + 4.f, bounds.y + bounds.height - 2.f, bounds.width - 8.f, 2.f},
            ResolveColor(ColorToken::AccentPrimary));
    } else if (hoverAnim > 0.01f) {
        context.DrawRect(
            bounds,
            Color::Lerp(
                ResolveColor(ColorToken::PanelBackground),
                ResolveColor(ColorToken::HoverBackground),
                hoverAnim));
    }
    const auto color = active ? ResolveColor(ColorToken::TextPrimary)
                              : ResolveColor(ColorToken::TextSecondary);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(label),
        Point{bounds.x + 10.f, bounds.y + (bounds.height - fontSize) * 0.5f},
        color,
        fontSize,
        false);
}

void PaintSectionCard(PaintContext& context, const Rect& bounds) {
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    context.DrawRoundedRect(bounds, ResolveColor(ColorToken::CardBackground), radius);
    context.DrawRoundedRectOutline(bounds, ResolveColor(ColorToken::BorderSubtle), 1.f, radius);
}

void PaintSectionTitle(PaintContext& context, const Rect& bounds, std::string_view title) {
    const float fontSize = ResolveMetric(MetricToken::TextSizeBody) * UiScale();
    context.DrawText(
        std::string(title),
        Point{bounds.x, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        true);
}

void PaintSoftSeparator(PaintContext& context, const Rect& bounds) {
    context.DrawRect(bounds, ResolveColor(ColorToken::Separator));
}

void PaintChip(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    const char* iconHook,
    bool selected,
    float hoverAnim)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::ControlBackground);
    if (selected) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::AccentPrimary), 0.35f);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), hoverAnim);
    }
    context.DrawRoundedRect(bounds, bg, radius);
    context.DrawRoundedRectOutline(
        bounds,
        selected ? ResolveColor(ColorToken::AccentPrimary) : ResolveColor(ColorToken::BorderSubtle),
        1.f,
        radius);

    float textX = bounds.x + 8.f;
    if (iconHook && iconHook[0] != '\0') {
        const float iconSize = static_cast<float>(
            we::runtime::kindui::IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
        const Rect iconBand{bounds.x + 6.f, bounds.y, iconSize, bounds.height};
        we::runtime::kindui::IconPainter::DrawIcon(
            context,
            iconHook,
            we::runtime::kindui::IconMetrics::PlaceGlyphCentered(iconBand, iconSize),
            selected ? ResolveColor(ColorToken::AccentPrimary) : ResolveColor(ColorToken::IconPrimary));
        textX = bounds.x + 6.f + iconSize + 6.f;
    }
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(label),
        Point{textX, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

void PaintPropertyLabel(PaintContext& context, const Rect& bounds, std::string_view label) {
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(label),
        Point{bounds.x, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextSecondary),
        fontSize,
        false);
}

void PaintField(
    PaintContext& context,
    const Rect& bounds,
    std::string_view value,
    bool focused,
    bool hovered)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::InputBackground);
    if (focused) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::AccentPrimary), 0.12f);
    } else if (hovered) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), 0.5f);
    }
    context.DrawRoundedRect(bounds, bg, radius);
    context.DrawRoundedRectOutline(
        bounds,
        focused ? ResolveColor(ColorToken::BorderFocused) : ResolveColor(ColorToken::BorderSubtle),
        1.f,
        radius);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(value),
        Point{bounds.x + 8.f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

void PaintToggle(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    bool on,
    float hoverAnim)
{
    const float trackW = 34.f * UiScale();
    const float trackH = 18.f * UiScale();
    const float trackX = bounds.x;
    const float trackY = bounds.y + (bounds.height - trackH) * 0.5f;
    const Rect track{trackX, trackY, trackW, trackH};
    Color trackColor = on ? ResolveColor(ColorToken::AccentPrimary)
                          : ResolveColor(ColorToken::ControlBackground);
    if (hoverAnim > 0.01f && !on) {
        trackColor = Color::Lerp(trackColor, ResolveColor(ColorToken::HoverBackground), hoverAnim);
    }
    context.DrawRoundedRect(track, trackColor, trackH * 0.5f);
    const float knob = trackH - 4.f;
    const float knobX = on ? (track.x + track.width - knob - 2.f) : (track.x + 2.f);
    context.DrawRoundedRect(
        Rect{knobX, track.y + 2.f, knob, knob},
        ResolveColor(ColorToken::TextPrimary),
        knob * 0.5f);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(label),
        Point{track.x + track.width + 10.f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

void PaintPrimaryButton(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::ButtonPrimaryBackground);
    bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonPrimaryHover), hoverAnim);
    bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonPrimaryPressed), pressAnim);
    context.DrawRoundedRect(bounds, bg, radius);
    const float fontSize = ResolveMetric(MetricToken::TextSizeBody) * UiScale();
    const float textW = context.GetTextWidth(std::string(label), fontSize, true);
    context.DrawText(
        std::string(label),
        Point{bounds.x + (bounds.width - textW) * 0.5f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextOnAccent),
        fontSize,
        true);
}

void PaintSecondaryButton(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::ControlBackground);
    bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), hoverAnim * 0.7f + pressAnim * 0.3f);
    context.DrawRoundedRect(bounds, bg, radius);
    context.DrawRoundedRectOutline(bounds, ResolveColor(ColorToken::BorderSubtle), 1.f, radius);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    const float textW = context.GetTextWidth(std::string(label), fontSize, false);
    context.DrawText(
        std::string(label),
        Point{bounds.x + (bounds.width - textW) * 0.5f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

void PaintDangerButton(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    float hoverAnim,
    float pressAnim)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::ButtonDangerBackground);
    bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonDangerHover), hoverAnim);
    bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonDangerPressed), pressAnim);
    context.DrawRoundedRect(bounds, bg, radius);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    const float textW = context.GetTextWidth(std::string(label), fontSize, true);
    context.DrawText(
        std::string(label),
        Point{bounds.x + (bounds.width - textW) * 0.5f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextOnAccent),
        fontSize,
        true);
}

void PaintInfoValue(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    std::string_view value)
{
    PaintPropertyLabel(context, Rect{bounds.x, bounds.y, LabelColumnWidth(), bounds.height}, label);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(value),
        Point{bounds.x + LabelColumnWidth() + 8.f, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

} // namespace we::editor::terrain::LandscapePanelChrome
