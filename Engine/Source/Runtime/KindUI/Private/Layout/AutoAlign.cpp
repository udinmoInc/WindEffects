// ==============================================================================
// WindEffects — KindUI — AutoAlign
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/AutoAlign.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

float AutoAlign::ComputeVerticalCenter(const Rect& container, float contentHeight) {
    return container.y + (container.height - contentHeight) * 0.5f;
}

float AutoAlign::ComputeHorizontalAlign(const Rect& container, float contentWidth, HorizontalAlignment align) {
    switch (align) {
        case HorizontalAlignment::Center:
            return container.x + (container.width - contentWidth) * 0.5f;
        case HorizontalAlignment::Right:
            return container.x + container.width - contentWidth;
        case HorizontalAlignment::Left:
        default:
            return container.x;
    }
}

float AutoAlign::ComputeBaselineY(float centerY, float fontSizePx) {
    // 0.61 * fontSizePx aligns the visual midline of font characters with centerY
    // (accounting for font ascender = 0.96 * fontSizePx and cap-height = 0.70 * fontSizePx)
    return centerY - fontSizePx * 0.61f;
}

float AutoAlign::AlignTextTopAtCenterY(float centerY, float fontSizePx) {
    return ComputeBaselineY(centerY, fontSizePx);
}

float AutoAlign::AlignTextTopY(const Rect& bounds, float fontSizePx) {
    const float centerY = bounds.y + bounds.height * 0.5f;
    return ComputeBaselineY(centerY, fontSizePx);
}

Rect AutoAlign::NormalizeIconBounds(const Rect& controlBounds, float iconSizePx) {
    return IconMetrics::PlaceGlyphCentered(controlBounds, iconSizePx);
}

AutoAlign::IconTextResult AutoAlign::ComputeIconTextLayout(
    const Rect& container,
    float iconSizePx,
    bool hasIcon,
    std::string_view text,
    float fontSizePx,
    float gapPx)
{
    if (gapPx < 0.0f) {
        gapPx = ResolveMetric(MetricToken::Space1);
    }

    const float textW = text.empty() ? 0.0f : TextMetrics::MeasureWidth(text, fontSizePx);
    const float iconW = hasIcon ? iconSizePx : 0.0f;
    const bool hasBoth = hasIcon && !text.empty();
    const float totalW = iconW + (hasBoth ? gapPx : 0.0f) + textW;

    const float centerY = container.y + container.height * 0.5f;
    IconTextResult res{};
    res.totalWidth = totalW;
    res.totalHeight = (std::max)(hasIcon ? iconSizePx : 0.0f, fontSizePx * 1.333f);

    float curX = container.x;
    if (hasIcon) {
        res.iconRect = Rect{ curX, centerY - iconSizePx * 0.5f, iconSizePx, iconSizePx };
        curX += iconSizePx + (hasBoth ? gapPx : 0.0f);
    }
    if (!text.empty()) {
        res.textPos = Point{ curX, ComputeBaselineY(centerY, fontSizePx) };
    }

    return res;
}

AutoAlign::LabelControlResult AutoAlign::ComputeLabelControlLayout(
    const Rect& container,
    float labelWidth,
    float controlWidth,
    float controlHeight,
    float fontSizePx)
{
    const float centerY = container.y + container.height * 0.5f;
    LabelControlResult res{};
    res.labelWidth = labelWidth;
    res.labelPos = Point{ container.x, ComputeBaselineY(centerY, fontSizePx) };
    const float ctrlY = centerY - controlHeight * 0.5f;
    res.controlRect = Rect{ container.x + labelWidth, ctrlY, controlWidth, controlHeight };
    return res;
}

AutoAlign::GroupCenterResult AutoAlign::ComputeGroupCenterLayout(
    const Rect& container,
    float iconSizePx,
    bool hasIcon,
    std::string_view text,
    float fontSizePx,
    float chevronSizePx,
    bool hasChevron,
    float gapPx)
{
    if (gapPx < 0.0f) {
        gapPx = ResolveMetric(MetricToken::Space1);
    }

    const float textW = text.empty() ? 0.0f : TextMetrics::MeasureWidth(text, fontSizePx);
    float totalW = 0.0f;

    if (hasIcon) totalW += iconSizePx;
    if (hasIcon && (!text.empty() || hasChevron)) totalW += gapPx;
    if (!text.empty()) totalW += textW;
    if (!text.empty() && hasChevron) totalW += gapPx;
    if (hasChevron) totalW += chevronSizePx;

    const float centerY = container.y + container.height * 0.5f;
    const float groupX = container.x + (std::max)(0.0f, (container.width - totalW) * 0.5f);

    GroupCenterResult res{};
    res.totalWidth = totalW;
    res.groupBounds = Rect{ groupX, container.y, totalW, container.height };

    float curX = groupX;
    if (hasIcon) {
        res.iconRect = Rect{ curX, centerY - iconSizePx * 0.5f, iconSizePx, iconSizePx };
        curX += iconSizePx + ((!text.empty() || hasChevron) ? gapPx : 0.0f);
    }
    if (!text.empty()) {
        res.textPos = Point{ curX, ComputeBaselineY(centerY, fontSizePx) };
        curX += textW + (hasChevron ? gapPx : 0.0f);
    }
    if (hasChevron) {
        res.chevronRect = Rect{ curX, centerY - chevronSizePx * 0.5f, chevronSizePx, chevronSizePx };
    }

    return res;
}

AutoAlign::InputContentResult AutoAlign::ComputeInputContentLayout(
    const Rect& container,
    float iconSizePx,
    bool hasSearchIcon,
    std::string_view text,
    float fontSizePx,
    bool showClearBtn,
    float padH)
{
    const float gap = ResolveMetric(MetricToken::Space1);
    const float centerY = container.y + container.height * 0.5f;
    InputContentResult res{};

    float startX = container.x + padH;
    float endX = container.x + container.width - padH;

    if (hasSearchIcon) {
        res.iconRect = Rect{ startX, centerY - iconSizePx * 0.5f, iconSizePx, iconSizePx };
        startX += iconSizePx + gap;
    }

    if (showClearBtn) {
        endX -= iconSizePx;
        res.clearBtnRect = Rect{ endX, centerY - iconSizePx * 0.5f, iconSizePx, iconSizePx };
        endX -= gap;
    }

    res.textPos = Point{ startX, ComputeBaselineY(centerY, fontSizePx) };
    res.textMaxW = (std::max)(0.0f, endX - startX);

    return res;
}

AutoAlign::TreeRowResult AutoAlign::ComputeTreeRowLayout(
    const Rect& rowBounds,
    float indentWidth,
    int depth,
    bool hasExpander,
    float expanderSizePx,
    bool hasIcon,
    float iconSizePx,
    std::string_view labelText,
    float fontSizePx,
    bool hasEye,
    bool hasLock)
{
    const float gap = ResolveMetric(MetricToken::Space1);
    const float centerY = rowBounds.y + rowBounds.height * 0.5f;
    TreeRowResult res{};

    const float padLeft = ResolveMetric(MetricToken::Space2);
    const float padRight = ResolveMetric(MetricToken::Space2);
    float startX = rowBounds.x + padLeft + static_cast<float>(depth) * indentWidth;
    float endX = rowBounds.x + rowBounds.width - padRight;

    if (hasLock) {
        const float lockW = 14.0f;
        endX -= lockW;
        res.lockRect = Rect{ endX, centerY - lockW * 0.5f, lockW, lockW };
        endX -= gap;
    }

    if (hasEye) {
        const float eyeW = 14.0f;
        endX -= eyeW;
        res.eyeRect = Rect{ endX, centerY - eyeW * 0.5f, eyeW, eyeW };
        endX -= gap;
    }

    if (hasExpander) {
        res.expanderRect = Rect{ startX, centerY - expanderSizePx * 0.5f, expanderSizePx, expanderSizePx };
        startX += expanderSizePx + gap;
    }

    if (hasIcon) {
        res.iconRect = Rect{ startX, centerY - iconSizePx * 0.5f, iconSizePx, iconSizePx };
        startX += iconSizePx + gap;
    }

    res.textX = startX;
    res.labelPos = Point{ startX, ComputeBaselineY(centerY, fontSizePx) };
    res.textMaxW = (std::max)(0.0f, endX - startX);

    return res;
}

} // namespace we::runtime::kindui
