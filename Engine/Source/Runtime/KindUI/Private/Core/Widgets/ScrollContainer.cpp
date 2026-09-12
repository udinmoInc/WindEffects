// ==============================================================================
// WindEffects — KindUI — ScrollContainer
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/ScrollContainer.h"
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
    float contentH = m_Geometry.height;
    if (m_ContentWidget) {
        const Size contentSize = m_ContentWidget->Measure(Size{ m_Geometry.width, 1e9f });
        contentH = contentSize.height;
    }
    const float uiScale = (std::max)(1.0f, DPIContext::GetScale());
    m_ScrollMetrics = m_Scroll.UpdateMetrics(m_Geometry, m_Geometry.height, contentH, uiScale);
}

Size ScrollContainer::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ScrollContainer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    SyncScroll();
    if (m_ContentWidget) {
        const Rect contentRect{ allottedRect.x, allottedRect.y - m_Scroll.offset, allottedRect.width,
            m_Geometry.height };
        m_ContentWidget->Arrange(contentRect);
    }
}

void ScrollContainer::Paint(PaintContext& context) {
    SyncScroll();
    context.PushClipRect(m_ScrollMetrics.viewport);
    if (m_ContentWidget) {
        m_ContentWidget->Paint(context);
    }
    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

void ScrollContainer::OnMouseWheel(const MouseEvent& event) {
    SyncScroll();
    if (ScrollViewport::NeedsScrollbar(m_ScrollMetrics.viewport.height, m_Geometry.height)) {
        m_Scroll.ApplyWheel(event.deltaY, 36.0f, m_ScrollMetrics.viewport.height, m_Geometry.height);
        InvalidateLayout();
        InvalidatePaint();
    }
}

bool ScrollContainer::CanReceiveMouseWheelAt(const Point& pos) const {
    return ScrollViewport::CanReceiveWheelAt(m_Geometry, m_ScrollMetrics.viewport, m_ScrollMetrics, pos);
}

} // namespace we::runtime::kindui
