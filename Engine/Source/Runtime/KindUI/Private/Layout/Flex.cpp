#include "KindUI/Layout/Flex.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {
namespace {

float EffectiveFlexGrow(const Widget& child, bool row) {
    float grow = child.GetFlexGrow();
    // VerticalAlignment::Fill on a Column child (main axis) means consume free space.
    // Do not mirror HorizontalAlignment::Fill on Row — that is the default cross-axis
    // stretch hint and must not force every row child to grow.
    if (grow <= 0.0f && !row && child.GetVerticalAlignment() == VerticalAlignment::Fill) {
        grow = 1.0f;
    }
    return grow;
}

// Flex base size on the main axis. Grow items start at their minimum (usually 0)
// so they do not each claim 100% of availableSize during Measure and then get
// flex-shrinked to empty rectangles — the KindUI declarative layout regression.
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

    // Re-measure against the final content box so text wrapping and fill-width
    // controls see the real cross-axis constraint before basis resolution.
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
            if (shrink > 0.0f) {
                item.mainSize = std::max(0.0f, item.mainSize - deficit * (shrink / totalShrink));
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

        switch (m_Align) {
        case AlignItems::Stretch:
            crossSize = crossAvail;
            break;
        case AlignItems::Center:
            crossPos += (crossAvail - item.crossSize) * 0.5f;
            break;
        case AlignItems::End:
            crossPos += crossAvail - item.crossSize;
            break;
        case AlignItems::Start:
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

        Rect childRect;
        if (row) {
            childRect = { mainCursor, crossPos, item.mainSize, crossSize };
        } else {
            childRect = { crossPos, mainCursor, crossSize, item.mainSize };
        }

        item.widget->Arrange(childRect);
        mainCursor += item.mainSize + item.marginMainEnd;
    }
}

void Flex::Paint(PaintContext& context) {
    ClearPaintDirty();
    if (m_HasBackground) {
        context.DrawRect(m_Geometry, m_Background);
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
