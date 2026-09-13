// ==============================================================================
// WindEffects — KindUI — LayoutMetrics
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/LayoutMetrics.h"

#include "KindUI/Layout/AutoAlign.h"
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

}

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
    return UnifiedToolbarRowHeight();
}

float SearchInputPaddingH() {
    return std::floor(7.0f * UiScale());
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
    const float scale = UiScale();
    const float rowH = ResolveMetric(MetricToken::FormRowHeight) * scale;
    return std::max(rowH, InputMinHeight());
}

float PropertySectionHeight() {
    return ResolveMetric(MetricToken::CategoryHeaderHeight) * UiScale();
}

float PropertyObjectHeaderHeight() {
    const float scale = UiScale();
    return ResolveMetric(MetricToken::ControlHeightCompact) * scale;
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
    return UnifiedCompactRowHeight();
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
    if (control) {
        ApplyInputMinSize(*control);
        PropertyPanelChrome::ConfigureFormRowChildren(*lbl, control.get(), 0);
        row->AddChild(lbl);
        row->AddChild(control);
    } else {
        PropertyPanelChrome::ConfigureFormRowChildren(*lbl, nullptr, 0);
        row->AddChild(lbl);
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
    // Use consistent padding for form columns (equal top/bottom spacing)
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

float AlignTextTopAtCenterY(float centerY, float fontSizePx) {
    return AutoAlign::AlignTextTopAtCenterY(centerY, fontSizePx);
}

float AlignTextTopY(const Rect& bounds, float fontSizePx) {
    return AutoAlign::AlignTextTopY(bounds, fontSizePx);
}

// ============================================================================
// Unified Row-Height System
// ============================================================================

float UnifiedRowHeight() {
    // Standard row height for most UI elements (property rows, list items, etc.)
    return ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float UnifiedCompactRowHeight() {
    // Compact row height for dense UI (toolbar controls, compact lists)
    return ResolveMetric(MetricToken::ControlHeightCompact) * UiScale();
}

float UnifiedSectionHeaderHeight() {
    // Section header height now matches property row height for consistency
    return ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float UnifiedToolbarRowHeight() {
    // Toolbar row height for toolbar buttons and controls
    return ResolveMetric(MetricToken::PanelToolbarHeight) * UiScale();
}

float UnifiedTabRowHeight() {
    // Tab row height for dock tabs and mode tabs
    return ResolveMetric(MetricToken::PanelTabHeight) * UiScale();
}

float UnifiedListItemHeight() {
    // List item height for tree views, lists, and arrays
    return ResolveMetric(MetricToken::ListRowHeight) * UiScale();
}

float UnifiedArrayItemHeight() {
    // Array item height (same as list items for consistency)
    return UnifiedListItemHeight();
}

float UnifiedNestedRowHeight() {
    // Nested row height (same as standard row for consistency)
    return UnifiedRowHeight();
}

void ApplyUnifiedRowHeight(Widget& widget) {
    const float rowH = UnifiedRowHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedCompactRowHeight(Widget& widget) {
    const float rowH = UnifiedCompactRowHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedSectionHeaderHeight(Widget& widget) {
    const float rowH = UnifiedSectionHeaderHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedToolbarRowHeight(Widget& widget) {
    const float rowH = UnifiedToolbarRowHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedTabRowHeight(Widget& widget) {
    const float rowH = UnifiedTabRowHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedListItemHeight(Widget& widget) {
    const float rowH = UnifiedListItemHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedArrayItemHeight(Widget& widget) {
    const float rowH = UnifiedArrayItemHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

void ApplyUnifiedNestedRowHeight(Widget& widget) {
    const float rowH = UnifiedNestedRowHeight();
    const Size current = widget.GetMinSize();
    widget.SetMinSize({ current.width, std::max(current.height, rowH) });
}

}

