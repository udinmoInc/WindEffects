#include "KindUI/Core/ControlChrome.h"
#include "LandscapePanelChrome.h"

#include "KindUI/Core/PropertyPanelChrome.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/Types.h"

#include <algorithm>

namespace we::editor::terrain::LandscapePanelChrome {
namespace {

using we::runtime::kindui::Color;
using we::runtime::kindui::ColorToken;
namespace ControlChrome = we::runtime::kindui::ControlChrome;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::ResolvedStyle;
using we::runtime::kindui::StyleRole;
using we::runtime::kindui::ResolveInteractiveBackground;
using we::runtime::kindui::ResolveMetric;
namespace PanelChromeNs = we::editor::panels::PanelChrome;

namespace PropertyPanelChrome = we::runtime::kindui::PropertyPanelChrome;

float UiScale() {
    return std::max(1.0f, we::runtime::kindui::DPIContext::GetScale());
}

} // namespace

float PanelPad() { return PanelChromeNs::PanelPaddingH(); }
float SectionGap() { return PropertyPanelChrome::FormStackGap(); }
float RowHeight() { return PropertyPanelChrome::RowHeight(); }
float ChipHeight() { return ResolveMetric(MetricToken::ToolbarLabeledHeight) * UiScale(); }
float TabBarHeight() { return ResolveMetric(MetricToken::NavigationButtonSize) * UiScale(); }
float PrimaryButtonHeight() { return ResolveMetric(MetricToken::PrimaryButtonHeight) * UiScale(); }
float LabelColumnWidth() { return PropertyPanelChrome::LabelColumnWidth(); }

void PaintPanelBackground(PaintContext& context, const Rect& bounds) {
    PanelChromeNs::PaintContentRegion(context, bounds);
}

void PaintTabBar(PaintContext& context, const Rect& bounds) {
    PanelChromeNs::PaintListLabelBand(context, bounds);
}

void PaintSectionCard(PaintContext& context, const Rect& bounds) {
    PanelChromeNs::PaintContentWell(context, bounds);
}

void PaintSoftSeparator(PaintContext& context, const Rect& bounds) {
    (void)context;
    (void)bounds;
}

void PaintTab(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    bool active,
    float hoverAnim)
{
    we::runtime::kindui::ControlChrome::InteractionState state{
        hoverAnim,
        0.0f,
        active,
        false,
        false
    };
    we::runtime::kindui::ControlChrome::PaintPanelTab(context, bounds, label, state);
}

void PaintSectionTitle(PaintContext& context, const Rect& bounds, std::string_view title) {
    PanelChromeNs::PaintListLabelBand(context, bounds);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCategory) * UiScale();
    const float padH = PropertyPanelChrome::RowPaddingH();
    context.DrawText(
        std::string(title),
        Point{bounds.x + padH, bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        true);
}

void PaintChip(
    PaintContext& context,
    const Rect& bounds,
    std::string_view label,
    we::runtime::kindui::WindIconRef icon,
    bool selected,
    float hoverAnim)
{
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * UiScale();
    Color bg = ResolveColor(ColorToken::ControlBackground);
    if (selected) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::AccentPrimary), 0.35f);
    } else {
        const Color interactive = ResolveInteractiveBackground(hoverAnim, 0.0f, false, ColorToken::ControlBackground);
        if (interactive.a > 0.001f) {
            bg = interactive;
        }
    }
    context.DrawRoundedRect(bounds, bg, radius);
    const float bevelStrength = std::max(0.15f, 1.0f - hoverAnim * 0.2f);
    ControlChrome::PaintSubtleDropShadow(context, bounds, radius, bevelStrength * 0.65f);
    ControlChrome::PaintRaisedBevel(context, bounds, radius, bevelStrength);
    if (selected || hoverAnim > 0.01f) {
        context.DrawRoundedRectOutline(
            bounds,
            selected ? ResolveColor(ColorToken::AccentPrimary) : ResolveColor(ColorToken::BorderSubtle),
            1.f,
            radius);
    }

    float textX = bounds.x + ResolveMetric(MetricToken::Space2) * UiScale();
    if (icon.IsValid()) {
        const float iconSize = 16.0f;
        const Rect iconBand{bounds.x + ResolveMetric(MetricToken::Space2) * UiScale(), bounds.y, iconSize, bounds.height};
        we::runtime::kindui::IconPainter::Draw(
            context, icon, we::runtime::kindui::IconMetrics::PlaceGlyphCentered(iconBand, 16u));
        textX = bounds.x + ResolveMetric(MetricToken::Space2) * UiScale() + iconSize + ResolveMetric(MetricToken::Space2) * UiScale();
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
    ControlChrome::InteractionState state{};
    state.focused = focused;
    state.hoverAnim = hovered ? 1.0f : 0.0f;
    ControlChrome::PaintInputFrame(context, bounds, state);
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * UiScale();
    context.DrawText(
        std::string(value),
        Point{bounds.x + ResolveMetric(MetricToken::Space2) * UiScale(), bounds.y + (bounds.height - fontSize) * 0.5f},
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
    const float trackW = ResolveMetric(MetricToken::ToggleTrackWidth) * UiScale();
    const float trackH = ResolveMetric(MetricToken::ToggleTrackHeight) * UiScale();
    const float trackX = bounds.x;
    const float trackY = bounds.y + (bounds.height - trackH) * 0.5f;
    const Rect track{trackX, trackY, trackW, trackH};
    Color trackColor = on ? ResolveColor(ColorToken::AccentPrimary)
                          : ResolveColor(ColorToken::ControlBackground);
    if (!on && hoverAnim > 0.01f) {
        trackColor = ResolveInteractiveBackground(hoverAnim, 0.0f, false, ColorToken::ControlBackground);
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
    if (pressAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonPrimaryPressed), pressAnim);
    } else if (hoverAnim > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::ButtonPrimaryHover), hoverAnim);
    }
    ControlChrome::PaintPanelButtonFace(context, bounds, bg, radius, hoverAnim, pressAnim, true);
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
    const float mix = hoverAnim * 0.7f + pressAnim * 0.3f;
    if (mix > 0.01f) {
        bg = Color::Lerp(bg, ResolveColor(ColorToken::HoverBackground), mix);
    }
    ControlChrome::PaintPanelButtonFace(context, bounds, bg, radius, hoverAnim, pressAnim, false);
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
    ControlChrome::InteractionState state{hoverAnim, pressAnim, false, false, false};
    ResolvedStyle base = ControlChrome::Role(StyleRole::ButtonDanger);
    base.cornerRadius = radius;
    ControlChrome::PaintDangerButton(context, bounds, base, state);
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
        Point{bounds.x + LabelColumnWidth() + PropertyPanelChrome::ValueColumnGap(), bounds.y + (bounds.height - fontSize) * 0.5f},
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        false);
}

} // namespace we::editor::terrain::LandscapePanelChrome
