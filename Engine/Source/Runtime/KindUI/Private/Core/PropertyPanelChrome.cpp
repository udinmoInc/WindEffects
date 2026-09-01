#include "KindUI/Core/PropertyPanelChrome.h"

#include "KindUI/Core/ControlChrome.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"

#include <algorithm>

namespace we::runtime::kindui {
namespace PropertyPanelChrome {

namespace {

float UiScale() {
    return std::max(1.0f, DPIContext::GetScale());
}

} // namespace

float ObjectHeaderHeight() {
    const float scale = UiScale();
    return (ResolveMetric(MetricToken::HeaderControlHeight) + ResolveMetric(MetricToken::Space1) * 2.0f) * scale;
}

float CategoryTabHeight() {
    return ResolveMetric(MetricToken::ControlHeightCompact) * UiScale();
}

float CategoryTabRowHeight() {
    const float scale = UiScale();
    return CategoryTabHeight() + ResolveMetric(MetricToken::Space1) * 2.0f * scale;
}

float SectionHeight() {
    return ResolveMetric(MetricToken::CategoryHeaderHeight) * UiScale();
}

float RowHeight() {
    return ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float LabelColumnWidth() {
    return ResolveMetric(MetricToken::PropertyLabelColumnWidth) * UiScale();
}

float RowPaddingH() {
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float ControlPaddingH() {
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float PropertyIndentStep() {
    return ResolveMetric(MetricToken::PropertyIndentStep) * UiScale();
}

float FormColumnPadding() {
    return RowPaddingH();
}

float FormStackGap() {
    return ResolveMetric(MetricToken::ContentGap) * UiScale();
}

float ValueColumnGap() {
    return ResolveMetric(MetricToken::Space1) * UiScale();
}

void ConfigureFormRowChildren(Widget& labelWidget, Widget* controlWidget, int depth) {
    const float labelW = LabelColumnWidth();
    const float valuePad = ControlPaddingH();
    const float padH = RowPaddingH();

    labelWidget.SetMinWidth(labelW);
    labelWidget.SetMaxWidth(labelW);
    labelWidget.SetFlexShrink(0.0f);
    labelWidget.SetFlexGrow(0.0f);
    labelWidget.SetMargin(Margin{ padH + static_cast<float>(depth) * PropertyIndentStep(), 0.0f, 0.0f, 0.0f });

    if (controlWidget) {
        controlWidget->SetFlexGrow(1.0f);
        controlWidget->SetFlexShrink(1.0f);
        controlWidget->SetMargin(Margin{ 0.0f, 0.0f, valuePad, 0.0f });
    }
}

PropertyRowLayout LayoutPropertyRow(const Rect& rowRect, int depth) {
    PropertyRowLayout layout;
    layout.row = rowRect;

    const float padH = RowPaddingH();
    const float indent = padH + static_cast<float>(depth) * PropertyIndentStep();
    const float labelW = LabelColumnWidth();
    const float valuePad = ControlPaddingH();

    layout.label = Rect{
        rowRect.x + indent,
        rowRect.y,
        std::max(0.0f, labelW - (indent - padH)),
        rowRect.height
    };

    const float valueX = rowRect.x + labelW;
    layout.value = Rect{
        valueX,
        rowRect.y,
        std::max(0.0f, rowRect.width - (valueX - rowRect.x) - valuePad),
        rowRect.height
    };
    return layout;
}

Rect LayoutPropertyControlRect(const Rect& valueRect) {
    const float controlH = ResolveMetric(MetricToken::ControlHeightCompact) * UiScale();
    const float y = valueRect.y + (valueRect.height - controlH) * 0.5f;
    return Rect{ valueRect.x, y, valueRect.width, controlH };
}

void PaintObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    WindIconRef icon,
    bool active) {
    (void)active;
    const float scale = UiScale();
    const float padH = ResolveMetric(MetricToken::Space2) * scale;
    const float iconSize = 16.0f;
    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty) * scale;
    const float centerY = rect.y + rect.height * 0.5f;

    if (icon.IsValid()) {
        IconPainter::Draw(
            context,
            icon,
            Rect{ rect.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize });
    }

    const float textX = rect.x + padH + (icon.IsValid() ? iconSize + ResolveMetric(MetricToken::Space1) * scale : 0.0f);
    const Color textColor = active ? ResolveColor(ColorToken::TextPrimary) : ResolveColor(ColorToken::TextSecondary);
    context.DrawText(
        std::string(title),
        Point{ textX, centerY - fontSize * 0.5f },
        textColor,
        fontSize,
        active);
}

void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    bool expanded,
    bool hovered,
    float indent) {
    const float scale = UiScale();
    const Color bg = hovered
        ? ResolveInteractiveBackground(1.0f, 0.0f, false, ColorToken::ListLabelBandBackground)
        : ResolveColor(ColorToken::ListLabelBandBackground);
    context.DrawRect(rect, bg);

    const float padH = RowPaddingH() + indent;
    const float chevronSize = 16.0f;
    const float fontSize = ResolveMetric(MetricToken::TextSizeCategory) * scale;
    const float centerY = rect.y + rect.height * 0.5f;

    const WindIconRef chevronIcon = expanded ? WindIcons::ChevronDown16 : WindIcons::ChevronRight16;
    IconPainter::Draw(context, chevronIcon, IconMetrics::CompactGlyphBand(rect, rect.x + padH));

    const float textX = rect.x + padH + chevronSize + ResolveMetric(MetricToken::Space1) * scale;
    context.DrawText(
        std::string(title),
        Point{ textX, centerY - fontSize * 0.5f },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        true);
}

void PaintPropertyRowLabel(
    PaintContext& context,
    const Rect& labelRect,
    std::string_view label,
    bool mixed) {
    const float scale = UiScale();
    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty) * scale;
    const float textY = labelRect.y + (labelRect.height - fontSize) * 0.5f;
    context.DrawText(
        std::string(label),
        Point{ labelRect.x, textY },
        mixed ? ResolveColor(ColorToken::AccentPrimary) : ResolveColor(ColorToken::TextSecondary),
        fontSize);
}

void PaintCategoryTab(
    PaintContext& context,
    const Rect& rect,
    std::string_view label,
    bool active,
    bool hovered) {
    const float scale = UiScale();
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * scale;
    const float fontSize = ResolveMetric(MetricToken::TextSizeCaption) * scale;

    if (active) {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::AccentPrimary), radius);
    } else if (hovered) {
        ControlChrome::PaintInteractiveFill(
            context,
            rect,
            radius,
            1.0f,
            0.0f,
            false,
            ColorToken::InputBackground);
    } else {
        context.DrawRoundedRect(rect, ResolveColor(ColorToken::InputBackground), radius);
    }

    const float textW = context.GetTextWidth(std::string(label), fontSize);
    const float textX = rect.x + (rect.width - textW) * 0.5f;
    const float textY = rect.y + (rect.height - fontSize) * 0.5f;
    context.DrawText(
        std::string(label),
        Point{ textX, textY },
        active ? ResolveColor(ColorToken::TextOnAccent) : ResolveColor(ColorToken::TextSecondary),
        fontSize);
}

void PaintPropertyRowBackground(
    PaintContext& context,
    const Rect& rowRect,
    bool hovered,
    bool selected) {
    ControlChrome::InteractionState state;
    state.hoverAnim = hovered ? 1.0f : 0.0f;
    state.selected = selected;
    ControlChrome::PaintListRow(context, rowRect, state);
}

} // namespace PropertyPanelChrome
} // namespace we::runtime::kindui
