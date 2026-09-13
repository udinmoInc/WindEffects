// ==============================================================================
// WindEffects — KindUI — PropertyPanelChrome
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Layout/AutoAlign.h"

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
#include <cmath>

namespace we::runtime::kindui {
namespace PropertyPanelChrome {

namespace {

float UiScale() {
    return std::max(1.0f, DPIContext::GetScale());
}

}

float ObjectHeaderHeight() {
    const float scale = UiScale();
    const float titleRowH = ResolveMetric(MetricToken::ControlHeightCompact) * scale;
    return titleRowH;
}

float CategoryTabHeight() {
    return ResolveMetric(MetricToken::ControlHeightCompact) * UiScale();
}

float CategoryTabRowHeight() {
    const float scale = UiScale();
    return CategoryTabHeight() + ResolveMetric(MetricToken::Space1) * 2.0f * scale;
}

float SectionHeight() {
    // Section headers now match property row height for consistent vertical rhythm
    return ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float RowHeight() {
    // Direct metric call to avoid circular dependency
    return ResolveMetric(MetricToken::FormRowHeight) * UiScale();
}

float LabelColumnWidth() {
    return ResolveMetric(MetricToken::PropertyLabelColumnWidth) * UiScale();
}

float RowPaddingH() {
    // Consistent horizontal padding for rows and headers
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float ControlPaddingH() {
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float PropertyIndentStep() {
    return ResolveMetric(MetricToken::PropertyIndentStep) * UiScale();
}

float FormColumnPadding() {
    // Consistent padding for form columns (top, bottom, left, right)
    return ResolveMetric(MetricToken::Space2) * UiScale();
}

float FormStackGap() {
    // Consistent minimal gap for vertical rhythm
    return ResolveMetric(MetricToken::FormRowGap) * UiScale();
}

float ValueColumnGap() {
    return ResolveMetric(MetricToken::Space1) * UiScale();
}

float ActionIconSize() {
    return static_cast<float>(IconMetrics::kCompactGlyphPx) * UiScale();
}

float ActionSlotGap() {
    return ValueColumnGap();
}

float ActionStripWidth(const PropertyActionIcons& icons) {
    // Calculate width based on which actions are present
    int slotCount = 0;
    if (icons.undo) slotCount++;
    return ActionIconSize() * static_cast<float>(slotCount);
}

float DividerInset() {
    // Continuous column rules run edge-to-edge; callers that need a short
    // segment inside a rounded control may still query this (currently 0).
    return 0.0f;
}

void ConfigureFormRowChildren(Widget& labelWidget, Widget* controlWidget, int depth) {
    const float columnW = LabelColumnWidth();
    const float valuePad = ControlPaddingH();
    const float indent = static_cast<float>(depth) * PropertyIndentStep();

    // Depth may indent a label to express hierarchy, but it must never move
    // the value column. Keeping the combined indent + label width constant
    // makes every Inspector control begin on the same vertical grid line.
    const float labelW = std::max(0.0f, columnW - indent);

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

PropertyRowLayout LayoutPropertyRow(const Rect& rowRect, int depth, const PropertyActionIcons& icons, bool hasLockIcon,
    float labelColumnRatio) {
    PropertyRowLayout layout;
    layout.row = rowRect;

    const float scale = UiScale();
    const float padH = RowPaddingH();
    const float indent = padH + static_cast<float>(depth) * PropertyIndentStep();
    const float gap = ValueColumnGap();
    const float actionsW = ActionStripWidth(icons);

    const float labelX = rowRect.x + indent;
    const float actionsX = rowRect.x + rowRect.width - padH - actionsW;

    // 2nd vertical divider line (between Value and Action Strip at right edge)
    layout.valueRightDividerX = actionsX - gap * 0.5f;

    // Resizable control group width configured by labelColumnRatio with min-width safety clamping
    const float availableRowW = std::max(0.0f, rowRect.width - padH * 2.0f - actionsW - gap);

    const float minLabelW = 80.0f * scale;
    const float minValueW = 80.0f * scale;
    const float ratio = std::clamp(
        (labelColumnRatio > 0.0f) ? labelColumnRatio : 0.40f,
        std::min(0.85f, minLabelW / std::max(1.0f, availableRowW)),
        std::max(0.15f, 1.0f - (minValueW / std::max(1.0f, availableRowW)))
    );

    const float controlW = std::floor(availableRowW * (1.0f - ratio));

    const float controlRight = layout.valueRightDividerX - gap * 0.5f;
    const float valueX = std::max(labelX + minLabelW, controlRight - controlW);

    // 1st vertical divider line (between Label and Value)
    layout.columnDividerX = valueX - gap * 0.5f;

    // Position lock icon at the right edge of the label column if present
    float lockIconX = 0.0f;
    float lockIconSize = 0.0f;
    if (hasLockIcon) {
        lockIconSize = ActionIconSize();
        lockIconX = valueX - gap - lockIconSize;
    }

    layout.label = Rect{
        labelX,
        rowRect.y,
        std::max(0.0f, valueX - gap - labelX - (hasLockIcon ? (lockIconSize + 4.0f * scale) : 0.0f)),
        rowRect.height
    };

    layout.actions = Rect{
        actionsX,
        rowRect.y,
        actionsW,
        rowRect.height
    };

    layout.value = Rect{
        valueX,
        rowRect.y,
        std::max(0.0f, layout.valueRightDividerX - gap * 0.5f - valueX),
        rowRect.height
    };

    // Position lock icon
    if (hasLockIcon) {
        const float lockIconY = rowRect.y + (rowRect.height - lockIconSize) * 0.5f;
        layout.lockIcon = Rect{ lockIconX, lockIconY, lockIconSize, lockIconSize };
    } else {
        layout.lockIcon = Rect{};
    }

    return layout;
}

void PaintPropertySplitterHighlight(
    PaintContext& context,
    float x,
    float top,
    float bottom,
    bool isDragging,
    bool isHovered)
{
    if (!isDragging && !isHovered) {
        return;
    }
    const float scale = UiScale();
    const float width = (isDragging ? 3.0f : 2.0f) * scale;
    const Rect lineRect{
        std::floor(x - width * 0.5f),
        top,
        width,
        std::max(0.0f, bottom - top)
    };
    const auto colorToken = isDragging ? ColorToken::SelectedBackground : ColorToken::BorderFocus;
    context.DrawRect(lineRect, ResolveColor(colorToken));
}

Rect LayoutPropertyControlRect(const Rect& valueRect) {
    const float scale = UiScale();
    const float standardRowH = RowHeight();
    const float padding = ResolveMetric(MetricToken::Space1) * scale;

    if (valueRect.height > standardRowH + 2.0f * scale) {
        const float w = std::max(0.0f, valueRect.width - padding * 2.0f);
        const float x = valueRect.x + valueRect.width - padding - w;
        return Rect{ x, valueRect.y + padding, w, std::max(0.0f, valueRect.height - padding * 2.0f) };
    }

    const float controlH = (ResolveMetric(MetricToken::ControlHeightCompact) - ResolveMetric(MetricToken::Space1)) *
        scale;
    const float y = valueRect.y + (valueRect.height - controlH) * 0.5f;
    const float w = std::max(0.0f, valueRect.width - padding * 2.0f);
    const float x = valueRect.x + valueRect.width - padding - w;
    return Rect{ x, y, w, controlH };
}

PropertyActionLayout LayoutPropertyActions(const Rect& actionsRect) {
    PropertyActionLayout layout;
    layout.strip = actionsRect;

    const float iconSize = ActionIconSize();
    const float gap = ActionSlotGap();
    const float y = actionsRect.y + (actionsRect.height - iconSize) * 0.5f;

    auto slotAt = [&](int index) {
        return Rect{
            IconMetrics::SnapPx(actionsRect.x + static_cast<float>(index) * (iconSize + gap)),
            IconMetrics::SnapPx(y),
            iconSize,
            iconSize };
    };

    layout.undo = slotAt(0);
    layout.options = Rect{};
    return layout;
}

void PaintPropertyControlBorder(PaintContext& context, const Rect& valueRect) {
    const Rect controlRect = LayoutPropertyControlRect(valueRect);
    if (controlRect.IsEmpty()) {
        return;
    }

    // Inputs already own their fill and interaction state. This final pass
    // establishes a consistent Inspector grid edge across text, numeric, and
    // vector editors, using the theme's raised-control border color.
    const float scale = UiScale();
    const float radius = ResolveMetric(MetricToken::CornerRadiusSmall) * scale;
    const float width = std::max(1.0f, ResolveMetric(MetricToken::BorderWidth) * scale);
    context.DrawRoundedRectOutline(
        controlRect,
        ResolveColor(ColorToken::BorderLight),
        width,
        radius);
}

void PaintPropertyDivider(
    PaintContext& context,
    float x,
    float top,
    float bottom)
{
    if (bottom - top <= 1.0f) {
        return;
    }
    // Dark mid-tone input outline — darker than BorderLight, not near-black Separator.
    // Uses BorderSeparator from JSON theme configuration.
    // Use 1.0f thickness to match label text thickness instead of BorderWidth
    ControlChrome::PaintVerticalSeparator(
        context,
        x,
        top,
        bottom,
        1.0f,
        ColorToken::Separator);
}

void PaintPropertyHorizontalDivider(
    PaintContext& context,
    float y,
    float left,
    float right)
{
    const float width = right - left;
    if (width <= 1.0f) {
        return;
    }
    // Use 1.0f thickness to match label text thickness instead of BorderWidth
    const float borderW = std::max(1.0f, 1.0f * UiScale());
    const float snappedY = std::floor(y - borderW);
    context.DrawRect(
        Rect{ left, snappedY, width, borderW },
        ResolveColor(ColorToken::Separator));
}

void PaintPropertyColumnDivider(PaintContext& context, const PropertyRowLayout& layout) {
    // Full row height so adjacent property rows form one continuous column rule.
    PaintPropertyDivider(
        context,
        layout.columnDividerX,
        layout.row.y,
        layout.row.y + layout.row.height);
}

void PaintPropertyRowGrid(PaintContext& context, const PropertyRowLayout& layout) {
    // 1st vertical divider (between Label and Value)
    PaintPropertyColumnDivider(context, layout);

    // 2nd vertical divider (between Value and Actions - right after value input controls)
    if (layout.valueRightDividerX > 0.0f) {
        PaintPropertyDivider(
            context,
            layout.valueRightDividerX,
            layout.row.y,
            layout.row.y + layout.row.height);
    }

    // Horizontal row divider
    PaintPropertyHorizontalDivider(
        context,
        layout.row.y + layout.row.height,
        layout.row.x,
        layout.row.x + layout.row.width);
}

void PaintPropertyActions(
    PaintContext& context,
    const PropertyActionLayout& layout,
    const PropertyActionIcons& icons,
    float optionsHover)
{
    const float glyphPx = ActionIconSize();

    // Paint undo reset icon when property value has been changed
    if (icons.undo && icons.undoEnabled && !layout.undo.IsEmpty()) {
        IconPainter::Draw(
            context,
            WindIcons::Undo16,
            layout.undo,
            static_cast<uint32_t>(glyphPx),
            ResolveColor(ColorToken::IconPrimary));
    }
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
    const float textY = AutoAlign::AlignTextTopY(rect, fontSize);
    context.DrawText(
        text,
        Point{ textX, textY },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);
}

}

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
    const Rect titleRow{ rect.x, rect.y, rect.width, titleRowH };

    context.DrawSurface(titleRow, SurfaceRole::Panel, 0.0f, "DetailsObjectHeader");
    PaintInlineIconLabelRow(context, titleRow, displayName, icon, true);
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

    const WindIconRef chevronIcon = expanded ? WindIcons::TriangleDown16 : WindIcons::TriangleRight16;
    IconPainter::Draw(context, chevronIcon, IconMetrics::CompactGlyphBand(rect, rect.x + padH));

    const float textX = rect.x + padH + chevronSize + ResolveMetric(MetricToken::Space1) * scale;
    const float textY = AutoAlign::AlignTextTopY(rect, fontSize);
    context.DrawText(
        title,
        Point{ textX, textY },
        ResolveColor(ColorToken::TextPrimary),
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);

    const float borderW = std::max(1.0f, 1.0f * scale);
    const float snappedY = std::floor(rect.y + rect.height - borderW);
    context.DrawRect(
        Rect{ rect.x, snappedY, rect.width, borderW },
        ResolveColor(ColorToken::Separator));
}

void PaintPropertyRowLabel(
    PaintContext& context,
    const Rect& labelRect,
    std::string_view label,
    bool mixed) {
    (void)mixed;
    const float scale = UiScale();
    const float fontSize = ResolveMetric(MetricToken::TextSizeProperty) * scale;
    const float textY = AutoAlign::AlignTextTopY(labelRect, fontSize);

    std::string display(label);
    if (labelRect.width > 0.0f) {
        const float maxW = labelRect.width;
        if (context.GetTextWidth(display, fontSize) > maxW) {
            constexpr const char* kEllipsis = "...";
            std::string displayEllipsis = display + kEllipsis;
            while (display.size() > 1
                && context.GetTextWidth(displayEllipsis, fontSize) > maxW) {
                display.pop_back();
                displayEllipsis.pop_back();
            }
            display += kEllipsis;
        }
    }
    context.DrawText(
        display,
        Point{ labelRect.x, textY },
        ResolveColor(ColorToken::TextSecondary),
        fontSize);
}

void PaintPropertyLockIcon(
    PaintContext& context,
    const Rect& lockIconRect,
    bool locked) {
    if (lockIconRect.IsEmpty()) {
        return;
    }
    const float glyphPx = lockIconRect.width;
    WindIconRef lockIcon = locked ? WindIcons::Lock16 : WindIcons::LockOpen16;
    IconPainter::Draw(
        context,
        lockIcon,
        lockIconRect,
        static_cast<uint32_t>(glyphPx),
        ResolveColor(ColorToken::IconSecondary));
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
        context.DrawSurface(rect, SurfaceRole::Selected, ResolveMetric(MetricToken::CornerRadiusSmall) * scale,
            "CategoryTabActive");
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
        label,
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);
    const float textX = rect.x + (rect.width - textW) * 0.5f;
    const float textY = LayoutMetrics::AlignTextTopY(rect, fontSize);
    context.DrawText(
        label,
        Point{ textX, textY },
        active ? ResolveColor(ColorToken::TextPrimary) : ResolveColor(ColorToken::TextSecondary),
        fontSize,
        we::runtime::text::layout::FontWeight::Regular);
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

}
}

