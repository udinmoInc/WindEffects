// ==============================================================================
// WindEffects — KindUI — ScrollContainer
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/UI/ScrollContainer.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include <algorithm>

namespace we::runtime::kindui {

ScrollContainer::ScrollContainer(std::shared_ptr<Widget> contentWidget)
    : m_ContentWidget(std::move(contentWidget)) {
    if (m_ContentWidget) {
        AddChild(m_ContentWidget);
    }
}

ScrollContainer::~ScrollContainer() = default;

void ScrollContainer::SetContentWidget(std::shared_ptr<Widget> contentWidget) {
    if (m_ContentWidget != contentWidget) {
        if (m_ContentWidget) {
            RemoveChild(m_ContentWidget);
        }
        m_ContentWidget = std::move(contentWidget);
        if (m_ContentWidget) {
            AddChild(m_ContentWidget);
        }
        InvalidateLayout();
        InvalidatePaint();
    }
}

void ScrollContainer::SyncScroll() {
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    float contentH = m_Geometry.height;
    if (m_ContentWidget) {
        const Size contentSize = MeasureChild(m_ContentWidget, Size{ m_Geometry.width, 1e9f });
        contentH = contentSize.height;
    }
    m_ScrollMetrics = m_Scroll.UpdateMetrics(m_Geometry, m_Geometry.height, contentH, uiScale);
    if (m_ContentWidget && m_ScrollMetrics.showsScrollbar && m_ScrollMetrics.scrollbarWidth > 0.0f) {
        const float reducedW = (std::max)(0.0f, m_Geometry.width - m_ScrollMetrics.scrollbarWidth);
        const Size contentSize = MeasureChild(m_ContentWidget, Size{ reducedW, 1e9f });
        contentH = contentSize.height;
        m_ScrollMetrics = m_Scroll.UpdateMetrics(m_Geometry, m_Geometry.height, contentH, uiScale);
    }
}

Size ScrollContainer::Measure(const Size& availableSize) {
    if (!IsVisible()) {
        m_DesiredSize = {};
        return m_DesiredSize;
    }
    m_DesiredSize = availableSize;
    NoteMeasureCache(availableSize);
    return m_DesiredSize;
}

void ScrollContainer::Arrange(const Rect& allottedRect) {
    CommitGeometry(allottedRect);
    if (!IsVisible()) {
        return;
    }
    SyncScroll();
    if (m_ContentWidget) {
        float contentH = m_Geometry.height;
        const Size contentSize = m_ContentWidget->GetDesiredSize();
        if (contentSize.height > 0.0f) {
            contentH = contentSize.height;
        } else {
            contentH = MeasureChild(m_ContentWidget, Size{ m_ScrollMetrics.viewport.width, 1e9f }).height;
        }
        const Rect contentRect{
            m_ScrollMetrics.viewport.x,
            m_ScrollMetrics.viewport.y - m_Scroll.offset,
            m_ScrollMetrics.viewport.width,
            contentH
        };
        ArrangeChild(m_ContentWidget, contentRect);
    }
}

void ScrollContainer::Paint(PaintContext& context) {
    SyncScroll();
    context.PushClipRect(m_ScrollMetrics.viewport);
    if (m_ContentWidget) {
        m_ContentWidget->PaintSubtree(context);
    }
    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

void ScrollContainer::OnMouseWheel(const MouseEvent& event) {
    SyncScroll();
    if (m_ScrollMetrics.isScrollable) {
        m_Scroll.ApplyWheel(event.deltaY, 36.0f, m_ScrollMetrics.viewport.height, m_ScrollMetrics.viewport.height);
        // Scroll offset is arrangement-local — do not re-arm full shell layout.
        Arrange(m_Geometry);
        InvalidatePaint();
    }
}

bool ScrollContainer::CanReceiveMouseWheelAt(const Point& pos) const {
    return ScrollViewport::CanReceiveWheelAt(m_Geometry, m_ScrollMetrics.viewport, m_ScrollMetrics, pos);
}

} // namespace we::runtime::kindui
