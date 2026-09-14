// ==============================================================================
// WindEffects — KindUI — VirtualList
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/VirtualList.h"
#include "KindUI/Core/UIStateChange.h"
#include "KindUI/Theme/DesignToken.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "KindUI/Core/InputEvents.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

namespace {

constexpr float kHeightEpsilon = 0.5f;

} // namespace

VirtualList::VirtualList() {
    m_ItemHeight = ResolveMetric(MetricToken::ListRowHeight);
}

std::shared_ptr<VirtualList> MakeVirtualList() {
    auto list = std::make_shared<VirtualList>();
    list->SetItemHeight(ResolveMetric(MetricToken::ListRowHeight));
    return list;
}

std::shared_ptr<ListView> MakeListView() {
    return MakeVirtualList();
}

void VirtualList::InvalidateModel() {
    m_Slots.clear();
    m_Pool.Clear();
    m_Range = {};
    m_HeightPrefixDirty = true;
    InvalidateLayout();
}

void VirtualList::SetItemCount(size_t count) {
    if (m_ItemCount == count) {
        return;
    }
    m_ItemCount = count;
    m_HeightPrefixDirty = true;
    // Drop selection indices that fell off the end.
    for (auto it = m_Selected.begin(); it != m_Selected.end();) {
        if (*it >= m_ItemCount) {
            it = m_Selected.erase(it);
        } else {
            ++it;
        }
    }
    InvalidateLayout();
}

void VirtualList::SetItemHeight(float height) {
    const float next = (std::max)(1.0f, height);
    if (std::abs(next - m_ItemHeight) < 0.01f) {
        return;
    }
    m_ItemHeight = next;
    m_HeightPrefixDirty = true;
    InvalidateLayout();
}

void VirtualList::SetOverscan(size_t rows) {
    if (m_Overscan == rows) {
        return;
    }
    m_Overscan = rows;
    InvalidateLayout();
}

void VirtualList::SetItemFactory(ItemFactory factory) {
    m_ItemFactory = std::move(factory);
    InvalidateModel();
}

void VirtualList::SetItemFactory(std::function<std::shared_ptr<Widget>(size_t index)> factory) {
    if (!factory) {
        SetItemFactory(ItemFactory{});
        return;
    }
    SetItemFactory(ItemFactory{[factory = std::move(factory)](
                                    size_t index, std::shared_ptr<Widget> recycled) {
        (void)recycled;
        return factory(index);
    }});
}

void VirtualList::SetHeightProvider(HeightProvider provider) {
    m_HeightProvider = std::move(provider);
    m_HeightPrefixDirty = true;
    InvalidateLayout();
}

void VirtualList::RebuildHeightPrefix() {
    if (!m_HeightProvider) {
        m_HeightPrefix.clear();
        m_HeightPrefixDirty = false;
        return;
    }
    std::vector<float> heights(m_ItemCount);
    for (size_t i = 0; i < m_ItemCount; ++i) {
        heights[i] = (std::max)(1.0f, m_HeightProvider(i));
    }
    we::runtime::kindui::RebuildHeightPrefix(heights, m_HeightPrefix);
    m_HeightPrefixDirty = false;
}

float VirtualList::ContentHeight() const {
    if (m_HeightProvider) {
        if (m_HeightPrefix.size() == m_ItemCount + 1) {
            return m_HeightPrefix.back();
        }
        return static_cast<float>(m_ItemCount) * m_ItemHeight;
    }
    return static_cast<float>(m_ItemCount) * m_ItemHeight;
}

float VirtualList::RowHeight(size_t index) const {
    if (m_HeightProvider && m_HeightPrefix.size() == m_ItemCount + 1 && index < m_ItemCount) {
        return m_HeightPrefix[index + 1] - m_HeightPrefix[index];
    }
    return m_ItemHeight;
}

float VirtualList::RowOffset(size_t index) const {
    if (m_HeightProvider && m_HeightPrefix.size() == m_ItemCount + 1 && index <= m_ItemCount) {
        return m_HeightPrefix[index];
    }
    return FixedRowOffset(index, m_ItemHeight);
}

size_t VirtualList::IndexAtY(float contentY) const {
    if (m_ItemCount == 0) {
        return 0;
    }
    if (m_HeightProvider && m_HeightPrefix.size() == m_ItemCount + 1) {
        const ListVisibleRange hit = ComputeVariableVisibleRange(contentY, 1.0f, m_HeightPrefix, 0);
        return hit.Empty() ? m_ItemCount - 1 : hit.first;
    }
    if (m_ItemHeight <= 0.0f) {
        return 0;
    }
    return (std::min)(m_ItemCount - 1, static_cast<size_t>((std::max)(0.0f, contentY) / m_ItemHeight));
}

void VirtualList::SetScrollOffset(float offset) {
    const float maxScroll = (std::max)(0.0f, ContentHeight() - m_Geometry.height);
    const float clamped = (std::clamp)(offset, 0.0f, maxScroll);
    if (std::abs(clamped - m_ScrollOffset) < kHeightEpsilon) {
        return;
    }
    const ListVisibleRange prev = m_Range;
    m_ScrollOffset = clamped;
    ListVisibleRange next{};
    if (m_HeightProvider) {
        if (m_HeightPrefixDirty) {
            // Const-cast-free: mark dirty for Arrange.
            next = prev;
        } else {
            next = ComputeVariableVisibleRange(
                m_ScrollOffset, m_Geometry.height, m_HeightPrefix, m_Overscan);
        }
    } else {
        next = ComputeFixedVisibleRange(
            m_ScrollOffset, m_Geometry.height, m_ItemCount, m_ItemHeight, m_Overscan);
    }
    if (next.first != prev.first || next.count != prev.count) {
        InvalidateLayout();
    } else {
        InvalidatePaint();
    }
}

void VirtualList::ScrollIntoView(size_t index) {
    if (index >= m_ItemCount || m_Geometry.height <= 0.0f) {
        return;
    }
    const float top = RowOffset(index);
    const float bottom = top + RowHeight(index);
    if (top < m_ScrollOffset) {
        SetScrollOffset(top);
    } else if (bottom > m_ScrollOffset + m_Geometry.height) {
        SetScrollOffset(bottom - m_Geometry.height);
    }
}

void VirtualList::SetSelectedIndex(size_t index, bool multi) {
    if (index >= m_ItemCount) {
        return;
    }
    if (!multi) {
        m_Selected.clear();
    }
    m_Selected.insert(index);
    m_AnchorIndex = index;
    if (m_OnSelectionChanged) {
        m_OnSelectionChanged();
    }
    UIStateChangeGate::Post(*this, StateChangeKind::Selection);
}

void VirtualList::ClearSelection() {
    if (m_Selected.empty()) {
        return;
    }
    m_Selected.clear();
    m_AnchorIndex = static_cast<size_t>(-1);
    if (m_OnSelectionChanged) {
        m_OnSelectionChanged();
    }
    UIStateChangeGate::Post(*this, StateChangeKind::Selection);
}

void VirtualList::SyncVisibleWindow() {
    if (m_HeightPrefixDirty) {
        RebuildHeightPrefix();
    }

    ListVisibleRange next{};
    if (m_HeightProvider) {
        next = ComputeVariableVisibleRange(
            m_ScrollOffset, m_Geometry.height, m_HeightPrefix, m_Overscan);
    } else {
        next = ComputeFixedVisibleRange(
            m_ScrollOffset, m_Geometry.height, m_ItemCount, m_ItemHeight, m_Overscan);
    }

    if (!m_ItemFactory) {
        m_Slots.clear();
        m_Pool.Clear();
        m_Range = next;
        return;
    }

    const bool windowChanged =
        next.first != m_Range.first
        || next.count != m_Range.count
        || m_Slots.size() != next.count;

    if (!windowChanged && !m_Slots.empty()) {
        m_Range = next;
        return;
    }

    // Own rows only via recycle pool/slots — never Widget::m_Children.
    m_Pool.SyncWindow(next, m_Slots, m_ItemFactory);
    m_Range = next;
}

Size VirtualList::Measure(const Size& availableSize) {
    if (m_HeightPrefixDirty) {
        RebuildHeightPrefix();
    }
    const float contentH = ContentHeight();
    m_DesiredSize = {
        availableSize.width > 0.0f ? availableSize.width : GetMinSize().width,
        GetFlexGrow() > 0.0f ? GetMinSize().height : contentH
    };
    return m_DesiredSize;
}

void VirtualList::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    const float maxScroll = (std::max)(0.0f, ContentHeight() - allottedRect.height);
    m_ScrollOffset = (std::clamp)(m_ScrollOffset, 0.0f, maxScroll);

    SyncVisibleWindow();
    ClearLayoutDirty();

    for (auto& slot : m_Slots) {
        if (!slot.widget) {
            continue;
        }
        const float y = allottedRect.y + RowOffset(slot.dataIndex) - m_ScrollOffset;
        const float h = RowHeight(slot.dataIndex);
        Rect row{ allottedRect.x, y, allottedRect.width, h };
        (void)MeasureChild(slot.widget, Size{ allottedRect.width, h });
        ArrangeChild(slot.widget, row);
        slot.widget->ClearLayoutDirty();
    }
}

void VirtualList::Paint(PaintContext& context) {
    ClearPaintDirty();
    context.PushClipRect(m_Geometry);
    for (auto& slot : m_Slots) {
        if (slot.widget && slot.widget->IsVisible()) {
            slot.widget->PaintSubtree(context);
        }
    }
    context.PopClipRect();
}

void VirtualList::Tick(float deltaTime) {
    // Only active (visible+overscan) row widgets tick.
    for (auto& slot : m_Slots) {
        if (slot.widget && slot.widget->IsVisible()) {
            slot.widget->Tick(deltaTime);
        }
    }
}

void VirtualList::OnMouseWheel(const MouseEvent& event) {
    const float step = m_HeightProvider ? RowHeight(m_Range.first) : m_ItemHeight;
    SetScrollOffset(m_ScrollOffset - event.wheelDeltaY * step);
}

void VirtualList::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }
    const float localY = event.position.y - m_Geometry.y + m_ScrollOffset;
    const size_t index = IndexAtY(localY);
    if (index < m_ItemCount) {
        SetSelectedIndex(index, event.ctrlDown);
        ScrollIntoView(index);
    }
    for (auto& slot : m_Slots) {
        if (slot.widget && slot.widget->GetGeometry().Contains(event.position)) {
            slot.widget->OnMouseDown(event);
            break;
        }
    }
}

void VirtualList::OnKeyDown(const KeyEvent& event) {
    if (m_ItemCount == 0) {
        return;
    }
    size_t current = m_AnchorIndex < m_ItemCount ? m_AnchorIndex : 0;
    size_t next = current;
    const size_t page = m_Range.count > 0 ? m_Range.count : 1;

    using we::platform::KeyCode;
    switch (event.key) {
    case KeyCode::Up:
        next = current > 0 ? current - 1 : 0;
        break;
    case KeyCode::Down:
        next = (std::min)(m_ItemCount - 1, current + 1);
        break;
    case KeyCode::PageUp:
        next = current > page ? current - page : 0;
        break;
    case KeyCode::PageDown:
        next = (std::min)(m_ItemCount - 1, current + page);
        break;
    case KeyCode::Home:
        next = 0;
        break;
    case KeyCode::End:
        next = m_ItemCount - 1;
        break;
    default:
        for (auto& slot : m_Slots) {
            if (slot.widget) {
                slot.widget->OnKeyDown(event);
            }
        }
        return;
    }
    SetSelectedIndex(next, event.shiftDown);
    ScrollIntoView(next);
}


std::optional<Rect> VirtualList::GetHitTestClipRect() const {
    return m_Geometry;
}

bool VirtualList::CanReceiveMouseWheelAt(const Point& pos) const {
    return IsVisible() && m_Geometry.Contains(pos);
}

std::shared_ptr<Widget> VirtualList::HitTestPoint(const Point& pos, const Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if (clip && !clip->Contains(pos)) {
        return nullptr;
    }
    if (!m_Geometry.Contains(pos)) {
        return nullptr;
    }
    // Front-to-back over active slots only.
    for (auto it = m_Slots.rbegin(); it != m_Slots.rend(); ++it) {
        if (!it->widget) {
            continue;
        }
        if (auto hit = it->widget->HitTestPoint(pos, &m_Geometry)) {
            return hit;
        }
    }
    return shared_from_this();
}

} // namespace we::runtime::kindui
