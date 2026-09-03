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
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Tokens/TypographySpec.h"
#include "Text/Layout/TextStyle.h"

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
    const float titleRowH = ResolveMetric(MetricToken::ControlHeightCompact) * scale;
    return titleRowH + RowHeight();
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
    const float indent = static_cast<float>(depth) * PropertyIndentStep();

    // Fixed label column so every value control shares one left edge.
    // Horizontal inset comes from the form column padding — do not double-pad here.
    labelWidget.SetMinWidth(labelW);
    labelWidget.SetMaxWidth(labelW);
    labelWidget.SetFlexBasis(labelW);
    labelWidget.SetFlexShrink(0.0f);
    labelWidget.SetFlexGrow(0.0f);
    labelWidget.SetMargin(Margin{ indent, 0.0f, 0.0f, 0.0f });
    labelWidget.SetVerticalAlignment(VerticalAlignment::Center);

    if (controlWidget) {
        controlWidget->SetFlexGrow(1.0f);
        controlWidget->SetFlexShrink(1.0f);
        controlWidget->SetFlexBasis(0.0f);
        controlWidget->SetMargin(Margin{ 0.0f, 0.0f, valuePad, 0.0f });
        controlWidget->SetVerticalAlignment(VerticalAlignment::Center);
        controlWidget->SetMinWidth(0.0f);
    }
}

PropertyRowLayout LayoutPropertyRow(const Rect& rowRect, int depth) {
    PropertyRowLayout layout;
    layout.row = rowRect;

    const float padH = RowPaddingH();
    const float indent = padH + static_cast<float>(depth) * PropertyIndentStep();
    const float labelW = LabelColumnWidth();
    const float valuePad = ControlPaddingH();
    const float gap = ValueColumnGap();

    layout.label = Rect{
        rowRect.x + indent,
        rowRect.y,
        labelW,
        rowRect.height
    };

    const float valueX = layout.label.x + layout.label.width + gap;
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

namespace {

void PaintInlineIconLabelRow(
    PaintContext& context,
    const Rect& rect,
    std::string_view text,
    WindIconRef icon,
    bool emphasized) {
    const float scale = UiScale();
    const float padH = RowPaddingH();
    const float iconSize = 16.0f;
    const float gap = ResolveMetric(MetricToken::Space1) * scale;
    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty) * scale;
    const float centerY = rect.y + rect.height * 0.5f;

    if (icon.IsValid()) {
        IconPainter::Draw(
            context,
            icon,
            Rect{ rect.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize });
    }

    const float textX = rect.x + padH + (icon.IsValid() ? iconSize + gap : 0.0f);
    context.DrawText(
        std::string(text),
        Point{ textX, centerY - fontSize * 0.5f },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        emphasized
            ? we::runtime::text::layout::FontWeight::Medium
            : we::runtime::text::layout::FontWeight::Regular);
}

} // namespace

void PaintObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    WindIconRef icon,
    bool active) {
    PaintInlineIconLabelRow(
        context,
        rect,
        title,
        icon,
        active);
}

void PaintDetailsObjectHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view displayName,
    WindIconRef icon) {
    const float scale = UiScale();
    const float titleRowH = ResolveMetric(MetricToken::ControlHeightCompact) * scale;
    const float instanceRowH = RowHeight();

    const Rect titleRow{ rect.x, rect.y, rect.width, titleRowH };
    const Rect instanceRow{ rect.x, rect.y + titleRowH, rect.width, instanceRowH };

    PaintInlineIconLabelRow(context, titleRow, displayName, icon, true);

    ControlChrome::InteractionState state;
    state.selected = true;
    ControlChrome::PaintListRow(context, instanceRow, state);

    std::string instanceLabel = std::string(displayName);
    instanceLabel += " (Instance)";
    PaintInlineIconLabelRow(context, instanceRow, instanceLabel, icon, false);
}

void PaintSectionHeader(
    PaintContext& context,
    const Rect& rect,
    std::string_view title,
    bool expanded,
    bool hovered,
    float indent) {
    const float scale = UiScale();
    if (hovered) {
        ControlChrome::PaintInteractiveFill(
            context,
            rect,
            0.0f,
            1.0f,
            0.0f,
            false,
            SurfaceRole::PanelHeader);
    } else {
        context.DrawSurface(rect, SurfaceRole::PanelHeader, 0.0f, "SectionHeader");
    }

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
        we::runtime::text::layout::FontWeight::Medium);
}

void PaintPropertyRowLabel(
    PaintContext& context,
    const Rect& labelRect,
    std::string_view label,
    bool mixed) {
    const float scale = UiScale();
    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty) * scale;
    const float textY = labelRect.y + (labelRect.height - fontSize) * 0.5f;

    std::string display(label);
    if (labelRect.width > 0.0f) {
        const float maxW = labelRect.width;
        if (context.GetTextWidth(display, fontSize) > maxW) {
            constexpr const char* kEllipsis = "...";
            while (display.size() > 1
                && context.GetTextWidth(display + kEllipsis, fontSize) > maxW) {
                display.pop_back();
            }
            display += kEllipsis;
        }
    }

    context.PushClipRect(labelRect);
    context.DrawText(
        display,
        Point{ labelRect.x, textY },
        mixed ? ResolveColor(ColorToken::AccentPrimary) : ResolveColor(ColorToken::TextSecondary),
        fontSize);
    context.PopClipRect();
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
        context.DrawSurface(rect, SurfaceRole::Selected, ResolveMetric(MetricToken::CornerRadiusSmall) * scale, "CategoryTabActive");
    } else if (hovered) {
        ControlChrome::PaintInteractiveFill(
            context,
            rect,
            radius,
            1.0f,
            0.0f,
            false,
            SurfaceRole::Control);
    } else {
        context.DrawSurface(rect, SurfaceRole::Control, radius, "CategoryTab");
    }

    const float textW = context.GetTextWidth(
        std::string(label),
        fontSize,
        we::runtime::text::layout::FontWeight::Medium);
    const float textX = rect.x + (rect.width - textW) * 0.5f;
    const float textY = rect.y + (rect.height - fontSize) * 0.5f;
    context.DrawText(
        std::string(label),
        Point{ textX, textY },
        active ? ResolveColor(ColorToken::TextPrimary) : ResolveColor(ColorToken::TextSecondary),
        fontSize,
        we::runtime::text::layout::FontWeight::Medium);
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
