#include "KindUI/Widgets/VirtualList.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

std::shared_ptr<VirtualList> MakeVirtualList() {
    return std::make_shared<VirtualList>();
}

void VirtualList::SetItemCount(size_t count) {
    if (m_ItemCount == count) return;
    m_ItemCount = count;
    InvalidateLayout();
}

void VirtualList::SetItemHeight(float height) {
    m_ItemHeight = std::max(1.0f, height);
    InvalidateLayout();
}

void VirtualList::SetItemFactory(ItemFactory factory) {
    m_ItemFactory = std::move(factory);
    m_Cache.clear();
    InvalidateLayout();
}

void VirtualList::SetScrollOffset(float offset) {
    const float maxScroll = std::max(0.0f, static_cast<float>(m_ItemCount) * m_ItemHeight - m_Geometry.height);
    m_ScrollOffset = std::clamp(offset, 0.0f, maxScroll);
    InvalidateLayout();
}

void VirtualList::RebuildVisible() {
    ClearChildren();
    m_Cache.clear();
    if (!m_ItemFactory || m_ItemCount == 0 || m_ItemHeight <= 0.0f) {
        m_FirstVisible = 0;
        m_VisibleCount = 0;
        return;
    }

    m_FirstVisible = static_cast<size_t>(m_ScrollOffset / m_ItemHeight);
    const size_t visible = static_cast<size_t>(std::ceil(m_Geometry.height / m_ItemHeight)) + 1;
    m_VisibleCount = std::min(visible, m_ItemCount > m_FirstVisible ? m_ItemCount - m_FirstVisible : 0);

    for (size_t i = 0; i < m_VisibleCount; ++i) {
        auto item = m_ItemFactory(m_FirstVisible + i);
        if (!item) continue;
        m_Cache.push_back(item);
        AddChild(item);
    }
}

Size VirtualList::Measure(const Size& availableSize) {
    m_DesiredSize = {
        availableSize.width,
        std::min(availableSize.height, static_cast<float>(m_ItemCount) * m_ItemHeight)
    };
    return m_DesiredSize;
}

void VirtualList::Arrange(const Rect& allottedRect) {
    const bool geometryChanged =
        std::abs(m_Geometry.width - allottedRect.width) > 0.5f ||
        std::abs(m_Geometry.height - allottedRect.height) > 0.5f;
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    if (geometryChanged || m_Cache.empty()) {
        RebuildVisible();
    }

    const float y0 = allottedRect.y - std::fmod(m_ScrollOffset, m_ItemHeight);
    for (size_t i = 0; i < m_Cache.size(); ++i) {
        Rect row{
            allottedRect.x,
            y0 + static_cast<float>(i) * m_ItemHeight,
            allottedRect.width,
            m_ItemHeight
        };
        m_Cache[i]->Measure({ allottedRect.width, m_ItemHeight });
        m_Cache[i]->Arrange(row);
    }
}

void VirtualList::Paint(PaintContext& context) {
    ClearPaintDirty();
    for (auto& child : m_Cache) {
        if (child && child->IsVisible()) {
            child->Paint(context);
        }
    }
}

void VirtualList::OnMouseWheel(const MouseEvent& event) {
    SetScrollOffset(m_ScrollOffset - event.wheelDeltaY * m_ItemHeight);
}

} // namespace we::runtime::kindui
