#include "KindUI/Core/LayoutMetrics.h"

#include "KindUI/Core/DPIContext.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Widgets/Label.h"

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

float FormRowMinHeight() {
    return std::max(
        ResolvedStyleHeight(StyleRole::PropertyRow, MetricToken::FormRowHeight),
        InputMinHeight());
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
    row->Gap(ResolveMetric(MetricToken::Space2));
    row->SetFlexShrink(0.0f);
    ApplyFormRowMinSize(*row);

    const float labelW = ResolveMetric(MetricToken::PropertyLabelColumnWidth);
    auto lbl = std::make_shared<Label>(label, TypographyToken::PropertyLabel);
    lbl->SetMinWidth(labelW);
    lbl->SetMaxWidth(labelW);
    lbl->SetFlexShrink(0.0f);
    lbl->SetFlexGrow(0.0f);

    row->AddChild(lbl);

    if (control) {
        ApplyInputMinSize(*control);
        control->SetFlexGrow(1.0f);
        control->SetFlexShrink(0.0f);
        row->AddChild(control);
    }

    return row;
}

} // namespace we::runtime::kindui::LayoutMetrics
