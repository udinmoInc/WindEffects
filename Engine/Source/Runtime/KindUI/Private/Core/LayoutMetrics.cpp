#include "KindUI/Core/LayoutMetrics.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Widgets/TextBox.h"

#include "KindUI/Core/PropertyPanelChrome.h"

#include <algorithm>

namespace we::runtime::kindui::LayoutMetrics {
namespace {

float UiScale() {
    return std::max(1.0f, DPIContext::GetScale());
}

} // namespace

float ResolvedStyleHeight(StyleRole role, MetricToken fallbackToken) {
    const ResolvedStyle style = ThemeManager::Get().Resolve(role);
    if (style.height > 0.0f) {
        return style.height;
    }
    return ResolveMetric(fallbackToken) * UiScale();
}

float InputMinHeight() {
    return ResolvedStyleHeight(StyleRole::Input, MetricToken::SearchBoxHeight);
}

float SearchInputHeight() {
    return ResolvedStyleHeight(StyleRole::SearchBox, MetricToken::SearchBoxHeight);
}

float ToolbarSearchInputHeight() {
    return SearchInputHeight();
}

float SearchRowHeight() {
    return ResolveMetric(MetricToken::PanelToolbarHeight) * UiScale();
}

float SearchInputPaddingH() {
    return ResolveMetric(MetricToken::SpaceMD) * UiScale();
}

float SearchInputFontSize() {
    const ResolvedStyle style = ThemeManager::Get().Resolve(StyleRole::SearchBox);
    if (style.fontSize > 0.0f) {
        return style.fontSize;
    }
    return ResolveMetric(MetricToken::TextSizeSmall) * UiScale();
}

float SearchInputIconSize() {
    return ResolveMetric(MetricToken::IconSizeSearch);
}

Rect LayoutSearchInputRect(const Rect& allottedRect) {
    const float h = std::min(SearchInputHeight(), allottedRect.height);
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    return Rect{ allottedRect.x, y, allottedRect.width, h };
}

Rect LayoutToolbarSearchInputRect(const Rect& allottedRect) {
    const float h = std::min(ToolbarSearchInputHeight(), allottedRect.height);
    const float y = allottedRect.y + (allottedRect.height - h) * 0.5f;
    return Rect{ allottedRect.x, y, allottedRect.width, h };
}

float FormRowMinHeight() {
    return std::max(
        ResolvedStyleHeight(StyleRole::PropertyRow, MetricToken::FormRowHeight),
        InputMinHeight());
}

float PropertySectionHeight() {
    return PropertyPanelChrome::SectionHeight();
}

float PropertyObjectHeaderHeight() {
    return PropertyPanelChrome::ObjectHeaderHeight();
}

float PropertyCategoryTabRowHeight() {
    return PropertyPanelChrome::CategoryTabRowHeight();
}

float PropertyLabelColumnWidth() {
    return PropertyPanelChrome::LabelColumnWidth();
}

float PropertyRowHeight() {
    return PropertyPanelChrome::RowHeight();
}

float PropertyControlHeight() {
    return ResolveMetric(MetricToken::ControlHeightCompact) * UiScale();
}

Rect LayoutPropertyControlInRow(const Rect& valueRect) {
    return PropertyPanelChrome::LayoutPropertyControlRect(valueRect);
}

float ButtonMinHeight(StyleRole role) {
    return ResolvedStyleHeight(role, MetricToken::ButtonHeight);
}

void ApplyInputMinSize(Widget& widget) {
    const float minH = InputMinHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, minH) });
}

void ApplyFormRowMinSize(Widget& widget) {
    const float minH = FormRowMinHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, minH) });
}

void ApplyButtonMinSize(Widget& widget, StyleRole role) {
    const float minH = ButtonMinHeight(role);
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, minH) });
}

std::shared_ptr<Row> MakeFormRow(const std::string& label, const std::shared_ptr<Widget>& control) {
    auto row = MakeRow();
    row->Align(AlignItems::Center);
    row->Gap(PropertyPanelChrome::ValueColumnGap());
    row->SetFlexShrink(0.0f);
    row->SetHorizontalAlignment(HorizontalAlignment::Fill);
    ApplyFormRowMinSize(*row);

    auto lbl = std::make_shared<Label>(label, TypographyToken::PropertyLabel);
    PropertyPanelChrome::ConfigureFormRowChildren(*lbl, nullptr, 0);
    row->AddChild(lbl);

    if (control) {
        ApplyInputMinSize(*control);
        PropertyPanelChrome::ConfigureFormRowChildren(*lbl, control.get(), 0);
        row->AddChild(control);
    }

    return row;
}

std::shared_ptr<Row> MakeTextFormRow(
    const std::string& label,
    const std::string& value,
    std::function<void(std::string_view)> onCommit)
{
    auto input = std::make_shared<TextBox>(value, [onCommit](const std::string& v) {
        if (onCommit) {
            onCommit(v);
        }
    });
    return MakeFormRow(label, input);
}

void ConfigurePropertyFormColumn(Column& column) {
    const float pad = PropertyPanelChrome::FormColumnPadding();
    column.Align(AlignItems::Stretch);
    column.Padding(Margin{ pad, pad, pad, pad });
    column.Gap(PropertyPanelChrome::FormStackGap());
}

float FormChipButtonMinWidth() {
    return ResolveMetric(MetricToken::PrimaryButtonHeight) * UiScale()
        + ResolveMetric(MetricToken::Space2) * UiScale() * 2.0f;
}

float TextLineHeight(float fontSizePx) {
    return std::max(1.0f, fontSizePx) * kTextLineHeightRatio;
}

float AlignTextTopY(const Rect& bounds, float fontSizePx) {
    const float lineH = TextLineHeight(fontSizePx);
    return bounds.y + (bounds.height - lineH) * 0.5f;
}

float AlignTextTopAtCenterY(float centerY, float fontSizePx) {
    return centerY - TextLineHeight(fontSizePx) * 0.5f;
}

} // namespace we::runtime::kindui::LayoutMetrics
