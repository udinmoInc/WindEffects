// ==============================================================================
// WindEffects — KindUI — VirtualList
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/VirtualList.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

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

void VirtualList::SetItemCount(size_t count) {
    if (m_ItemCount == count) return;
    m_ItemCount = count;
    InvalidateLayout();
}

void VirtualList::SetItemHeight(float height) {
    const float next = std::max(1.0f, height);
    if (std::abs(next - m_ItemHeight) < 0.01f) return;
    m_ItemHeight = next;
    InvalidateLayout();
}

void VirtualList::SetItemFactory(ItemFactory factory) {
    m_ItemFactory = std::move(factory);
    m_Cache.clear();
    InvalidateLayout();
}

void VirtualList::SetScrollOffset(float offset) {
    const float maxScroll = std::max(0.0f, static_cast<float>(m_ItemCount) * m_ItemHeight - m_Geometry.height);
    const float clamped = std::clamp(offset, 0.0f, maxScroll);
    if (std::abs(clamped - m_ScrollOffset) < kHeightEpsilon) {
        return;
    }
    const size_t prevFirst = m_FirstVisible;
    m_ScrollOffset = clamped;
    const size_t nextFirst = (m_ItemHeight > 0.0f)
        ? static_cast<size_t>(m_ScrollOffset / m_ItemHeight)
        : 0;
    // Window change requires a layout pass to rebuild rows; same window only needs paint/arrange.
    if (nextFirst != prevFirst || m_Cache.empty()) {
        InvalidateLayout();
    } else {
        InvalidatePaint();
    }
}

void VirtualList::ComputeVisibleWindow(size_t& firstVisible, size_t& visibleCount) const {
    firstVisible = 0;
    visibleCount = 0;
    if (!m_ItemFactory || m_ItemCount == 0 || m_ItemHeight <= 0.0f || m_Geometry.height <= 0.0f) {
        return;
    }
    firstVisible = static_cast<size_t>(m_ScrollOffset / m_ItemHeight);
    const size_t visible = static_cast<size_t>(std::ceil(m_Geometry.height / m_ItemHeight)) + 1;
    visibleCount = std::min(visible, m_ItemCount > firstVisible ? m_ItemCount - firstVisible : 0);
}

void VirtualList::RebuildVisible() {
    // Silent structural rebuild: must not re-arm UIRepaintGate during Arrange.
    // Expand/collapse changes allotted height once; continuous InvalidateLayout here
    // was the panel-expansion rebuild loop.
    ClearChildrenSilent();
    m_Cache.clear();

    size_t first = 0;
    size_t visible = 0;
    ComputeVisibleWindow(first, visible);
    m_FirstVisible = first;
    m_VisibleCount = visible;
    if (visible == 0) {
        return;
    }

    for (size_t i = 0; i < visible; ++i) {
        auto item = m_ItemFactory(first + i);
        if (!item) {
            continue;
        }
        m_Cache.push_back(item);
        AddChildSilent(item);
    }
}

Size VirtualList::Measure(const Size& availableSize) {
    // Intrinsic height from item count; width follows available when known.
    // Do not claim availableSize.height — that overflows parent Flex and triggers
    // a shrink-to-zero pass on the whole shell.
    const float contentH = static_cast<float>(m_ItemCount) * m_ItemHeight;
    m_DesiredSize = {
        availableSize.width > 0.0f ? availableSize.width : GetMinSize().width,
        GetFlexGrow() > 0.0f ? GetMinSize().height : contentH
    };
    return m_DesiredSize;
}

void VirtualList::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    ClearLayoutDirty();

    size_t nextFirst = 0;
    size_t nextVisible = 0;
    ComputeVisibleWindow(nextFirst, nextVisible);
    const bool windowChanged =
        nextFirst != m_FirstVisible
        || nextVisible != m_VisibleCount
        || m_Cache.size() != nextVisible;

    // Rebuild only when the visible window actually changes. Never RequestLayout
    // from here — silent child swap keeps the expand settle path one-shot.
    if (m_Cache.empty() || windowChanged) {
        RebuildVisible();
        ClearLayoutDirty();
    } else {
        m_FirstVisible = nextFirst;
        m_VisibleCount = nextVisible;
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
        m_Cache[i]->ClearLayoutDirty();
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
