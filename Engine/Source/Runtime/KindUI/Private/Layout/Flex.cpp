// ==============================================================================
// WindEffects — KindUI — Flex
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/LayoutAssert.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {
namespace {

float EffectiveFlexGrow(const Widget& child, bool row) {
    float grow = child.GetFlexGrow();
    if (grow <= 0.0f && !row && child.GetVerticalAlignment() == VerticalAlignment::Fill) {
        grow = 1.0f;
    }
    return grow;
}

float ResolveMainBasis(const Widget& child, float desiredMain, bool row) {
    if (child.GetFlexBasis() >= 0.0f) {
        return child.GetFlexBasis();
    }
    if (EffectiveFlexGrow(child, row) > 0.0f) {
        return row ? child.GetMinSize().width : child.GetMinSize().height;
    }
    return desiredMain;
}

} // namespace

Flex::Flex(FlexDirection direction) : m_Direction(direction) {}

Flex& Flex::Style(std::string className) {
    SetStyleClass(std::move(className));
    return *this;
}

Flex& Flex::Gap(SpacingToken token) {
    return Gap(ResolveSpacing(token));
}

Flex& Flex::Padding(PaddingToken token) {
    return Padding(ResolvePadding(token));
}

Flex& Flex::Background(ColorToken token) {
    return Background(ResolveColor(token));
}

Flex& Flex::Radius(RadiusToken token) {
    return Radius(ResolveRadius(token));
}

Flex& Flex::Align(AlignItems a) {
    m_Align = a;
    switch (a) {
    case AlignItems::Start: m_AlignRule = AlignRule::Start; break;
    case AlignItems::End: m_AlignRule = AlignRule::End; break;
    case AlignItems::Center: m_AlignRule = AlignRule::Center; break;
    case AlignItems::Stretch: m_AlignRule = AlignRule::Stretch; break;
    }
    InvalidateLayout();
    return *this;
}

Flex& Flex::Align(AlignRule rule) {
    m_AlignRule = rule;
    switch (rule) {
    case AlignRule::Start: m_Align = AlignItems::Start; break;
    case AlignRule::End: m_Align = AlignItems::End; break;
    case AlignRule::Center:
    case AlignRule::Baseline:
    case AlignRule::IconText:
    case AlignRule::LabelControl:
    case AlignRule::GroupCenter:
        m_Align = AlignItems::Center;
        break;
    case AlignRule::Stretch:
        m_Align = AlignItems::Stretch;
        break;
    }
    InvalidateLayout();
    return *this;
}

Size Flex::Measure(const Size& availableSize) {

    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const Size contentAvail{
        std::max(0.0f, availableSize.width - padW),
        std::max(0.0f, availableSize.height - padH)
    };

    float main = 0.0f;
    float cross = 0.0f;
    bool first = true;
    const bool row = IsRow();

    for (const auto& child : m_Children) {
        if (!child || !child->IsVisible()) continue;

        Size desired = child->Measure(contentAvail);
        desired = child->ClampDesiredSize(desired);

        const float desiredMain = row ? desired.width : desired.height;
        const float desiredCross = row ? desired.height : desired.width;
        const float basisMain = ResolveMainBasis(*child, desiredMain, row);

        const Margin& margin = child->GetMargin();
        if (row) {
            if (!first) main += m_Gap;
            main += basisMain + margin.left + margin.right;
            cross = std::max(cross, desiredCross + margin.top + margin.bottom);
        } else {
            if (!first) main += m_Gap;
            main += basisMain + margin.top + margin.bottom;
            cross = std::max(cross, desiredCross + margin.left + margin.right);
        }
        first = false;
    }

    m_DesiredSize = row
        ? Size{ main + padW, cross + padH }
        : Size{ cross + padW, main + padH };
    return m_DesiredSize;
}

void Flex::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    const float padW = m_Padding.left + m_Padding.right;
    const float padH = m_Padding.top + m_Padding.bottom;
    const float contentW = std::max(0.0f, allottedRect.width - padW);
    const float contentH = std::max(0.0f, allottedRect.height - padH);
    const bool row = IsRow();

    for (const auto& child : m_Children) {
        if (!child || !child->IsVisible()) continue;
        child->Measure(Size{ contentW, contentH });
    }

    struct Item {
        std::shared_ptr<Widget> widget;
        Size desired{};
        float mainSize = 0.0f;
        float crossSize = 0.0f;
        float marginMainStart = 0.0f;
        float marginMainEnd = 0.0f;
        float marginCrossStart = 0.0f;
        float marginCrossEnd = 0.0f;
        float grow = 0.0f;
    };

    std::vector<Item> items;
    items.reserve(m_Children.size());
    float totalFixedMain = 0.0f;
    float totalGrow = 0.0f;
    float totalShrink = 0.0f;
    int visibleCount = 0;

    for (const auto& child : m_Children) {
        if (!child || !child->IsVisible()) continue;
        ++visibleCount;

        Item item;
        item.widget = child;
        item.desired = child->ClampDesiredSize(child->GetDesiredSize());
        item.grow = EffectiveFlexGrow(*child, row);
        const Margin& m = child->GetMargin();

        if (row) {
            item.marginMainStart = m.left;
            item.marginMainEnd = m.right;
            item.marginCrossStart = m.top;
            item.marginCrossEnd = m.bottom;
            item.mainSize = ResolveMainBasis(*child, item.desired.width, row);
            item.crossSize = item.desired.height;
        } else {
            item.marginMainStart = m.top;
            item.marginMainEnd = m.bottom;
            item.marginCrossStart = m.left;
            item.marginCrossEnd = m.right;
            item.mainSize = ResolveMainBasis(*child, item.desired.height, row);
            item.crossSize = item.desired.width;
        }

        totalFixedMain += item.mainSize + item.marginMainStart + item.marginMainEnd;
        totalGrow += item.grow;
        totalShrink += child->GetFlexShrink();
        items.push_back(item);
    }

    const float gaps = visibleCount > 1 ? m_Gap * static_cast<float>(visibleCount - 1) : 0.0f;
    const float containerMain = row ? contentW : contentH;
    float freeSpace = containerMain - totalFixedMain - gaps;

    if (freeSpace > 0.0f && totalGrow > 0.0f) {
        for (auto& item : items) {
            if (item.grow > 0.0f) {
                item.mainSize += freeSpace * (item.grow / totalGrow);
            }
        }
        freeSpace = 0.0f;
    } else if (freeSpace < 0.0f && totalShrink > 0.0f) {
        const float deficit = -freeSpace;
        for (auto& item : items) {
            const float shrink = item.widget->GetFlexShrink();
            if (shrink <= 0.0f) {
                continue;
            }
            const float minMain = row
                ? item.widget->GetMinSize().width
                : item.widget->GetMinSize().height;
            const float reduced = item.mainSize - deficit * (shrink / totalShrink);
            item.mainSize = std::max(minMain, reduced);
        }
        for (auto& item : items) {
            const float minMain = row
                ? item.widget->GetMinSize().width
                : item.widget->GetMinSize().height;
            if (minMain > 0.0f) {
                item.mainSize = std::max(item.mainSize, minMain);
            }
        }
        freeSpace = 0.0f;
    }

    float mainCursor = row
        ? allottedRect.x + m_Padding.left
        : allottedRect.y + m_Padding.top;

    float justifyExtra = 0.0f;
    float justifyGap = 0.0f;
    if (freeSpace > 0.0f && visibleCount > 0) {
        switch (m_Justify) {
        case JustifyContent::End:
            mainCursor += freeSpace;
            break;
        case JustifyContent::Center:
            mainCursor += freeSpace * 0.5f;
            break;
        case JustifyContent::SpaceBetween:
            if (visibleCount > 1) justifyGap = freeSpace / static_cast<float>(visibleCount - 1);
            break;
        case JustifyContent::SpaceAround:
            justifyExtra = freeSpace / static_cast<float>(visibleCount);
            mainCursor += justifyExtra * 0.5f;
            justifyGap = justifyExtra;
            break;
        case JustifyContent::SpaceEvenly:
            justifyExtra = freeSpace / static_cast<float>(visibleCount + 1);
            mainCursor += justifyExtra;
            justifyGap = justifyExtra;
            break;
        case JustifyContent::Start:
        default:
            break;
        }
    }

    const float crossContainer = row ? contentH : contentW;
    const float crossOrigin = row
        ? allottedRect.y + m_Padding.top
        : allottedRect.x + m_Padding.left;

    bool first = true;
    for (auto& item : items) {
        if (!first) {
            mainCursor += m_Gap + justifyGap;
        }
        first = false;

        mainCursor += item.marginMainStart;

        float crossSize = item.crossSize;
        float crossPos = crossOrigin + item.marginCrossStart;
        const float crossAvail = crossContainer - item.marginCrossStart - item.marginCrossEnd;

        switch (m_AlignRule) {
        case AlignRule::Stretch:
            crossSize = crossAvail;
            break;
        case AlignRule::Center:
        case AlignRule::IconText:
        case AlignRule::LabelControl:
        case AlignRule::Baseline:
        case AlignRule::GroupCenter:
            crossPos += (crossAvail - item.crossSize) * 0.5f;
            break;
        case AlignRule::End:
            crossPos += crossAvail - item.crossSize;
            break;
        case AlignRule::Start:
        default:
            break;
        }

        if (m_Align == AlignItems::Stretch) {
            if (row && item.widget->GetVerticalAlignment() != VerticalAlignment::Fill) {
                crossSize = item.crossSize;
                if (item.widget->GetVerticalAlignment() == VerticalAlignment::Center) {
                    crossPos = crossOrigin + item.marginCrossStart + (crossAvail - crossSize) * 0.5f;
                } else if (item.widget->GetVerticalAlignment() == VerticalAlignment::Bottom) {
                    crossPos = crossOrigin + item.marginCrossStart + (crossAvail - crossSize);
                }
            } else if (!row && item.widget->GetHorizontalAlignment() != HorizontalAlignment::Fill) {
                crossSize = item.crossSize;
                if (item.widget->GetHorizontalAlignment() == HorizontalAlignment::Center) {
                    crossPos = crossOrigin + item.marginCrossStart + (crossAvail - crossSize) * 0.5f;
                } else if (item.widget->GetHorizontalAlignment() == HorizontalAlignment::Right) {
                    crossPos = crossOrigin + item.marginCrossStart + (crossAvail - crossSize);
                }
            }
        }

        const float mainEnd = row
            ? allottedRect.x + allottedRect.width - m_Padding.right
            : allottedRect.y + allottedRect.height - m_Padding.bottom;
        const float minMain = row
            ? item.widget->GetMinSize().width
            : item.widget->GetMinSize().height;
        if (mainCursor + item.mainSize > mainEnd) {
            item.mainSize = std::max(minMain, mainEnd - mainCursor);
        }

        const float minCross = row
            ? item.widget->GetMinSize().height
            : item.widget->GetMinSize().width;
        if (minCross > 0.0f) {
            crossSize = std::max(crossSize, minCross);
        }

        Rect childRect;
        if (row) {
            childRect = { mainCursor, crossPos, item.mainSize, crossSize };
        } else {
            childRect = { crossPos, mainCursor, crossSize, item.mainSize };
        }

        if (item.mainSize >= 0.5f && crossSize >= 0.5f) {
            item.widget->Arrange(childRect);
            AssertLayoutRectValid("Flex.child", childRect, allottedRect);
            AssertMinSizeRespected(
                "Flex.child",
                childRect,
                item.widget->GetMinSize().width,
                item.widget->GetMinSize().height);
        } else {
            item.widget->Arrange(Rect{childRect.x, childRect.y, 0.0f, 0.0f});
        }
        mainCursor += item.mainSize + item.marginMainEnd;
    }
}

void Flex::Paint(PaintContext& context) {
    ClearPaintDirty();
    if (m_HasBackground) {
        if (m_Radius > 0.0f) {
            context.DrawRoundedRect(m_Geometry, m_Background, m_Radius);
        } else {
            context.DrawRect(m_Geometry, m_Background);
        }
    }
    for (auto& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Paint(context);
        }
    }
}

std::shared_ptr<Row> MakeRow() {
    return std::make_shared<Row>();
}

std::shared_ptr<Column> MakeColumn() {
    return std::make_shared<Column>();
}

} // namespace we::runtime::kindui

