// ==============================================================================
// WindEffects — KindUI — CollapsibleGroup
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Layout/CollapsibleGroup.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/UIRepaintGate.h"

namespace we::runtime::kindui {

CollapsibleGroup::CollapsibleGroup(std::string title, bool expanded)
    : m_Title(std::move(title)), m_Expanded(expanded) {
    m_ContentColumn = std::make_shared<Column>();
    m_ContentColumn->SetVisible(m_Expanded);
    AddChild(m_ContentColumn);
}

CollapsibleGroup::~CollapsibleGroup() = default;

void CollapsibleGroup::SetTitle(std::string title) {
    if (m_Title != title) {
        m_Title = std::move(title);
        InvalidatePaint();
    }
}

void CollapsibleGroup::SetExpanded(bool expanded) {
    if (m_Expanded != expanded) {
        m_Expanded = expanded;
        if (m_ContentColumn) {
            m_ContentColumn->SetVisible(m_Expanded);
        }
        UIRepaintGate::RequestLayoutReason("PanelExpand");
        UIRepaintGate::RequestPaintReason("PanelExpand");
        if (m_OnExpandedChanged) {
            m_OnExpandedChanged(m_Expanded);
        }
    }
}

void CollapsibleGroup::SetOnExpandedChanged(std::function<void(bool expanded)> cb) {
    m_OnExpandedChanged = std::move(cb);
}

void CollapsibleGroup::AddContentChild(const std::shared_ptr<Widget>& child) {
    if (m_ContentColumn) {
        m_ContentColumn->AddChild(child);
        InvalidateLayout();
    }
}

Size CollapsibleGroup::Measure(const Size& availableSize) {
    const float headerH = PropertyPanelChrome::SectionHeight();
    Size contentSize{ availableSize.width, 0.0f };
    if (m_Expanded && m_ContentColumn && m_ContentColumn->IsVisible()) {
        contentSize = m_ContentColumn->Measure(availableSize);
    }
    m_DesiredSize = Size{ availableSize.width, headerH + contentSize.height };
    return m_DesiredSize;
}

void CollapsibleGroup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float headerH = PropertyPanelChrome::SectionHeight();
    if (m_Expanded && m_ContentColumn && m_ContentColumn->IsVisible()) {
        const Rect contentRect{ allottedRect.x, allottedRect.y + headerH, allottedRect.width, allottedRect.height -
            headerH };
        m_ContentColumn->Arrange(contentRect);
    }
}

void CollapsibleGroup::Paint(PaintContext& context) {
    const float headerH = PropertyPanelChrome::SectionHeight();
    const Rect headerRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, headerH };
    PropertyPanelChrome::PaintSectionHeader(context, headerRect, m_Title, m_Expanded, m_HeaderHovered, 0.0f);

    if (m_Expanded && m_ContentColumn && m_ContentColumn->IsVisible()) {
        m_ContentColumn->Paint(context);
    }
}

void CollapsibleGroup::OnMouseMove(const MouseEvent& event) {
    const float headerH = PropertyPanelChrome::SectionHeight();
    const Rect headerRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, headerH };
    const bool hovered = headerRect.Contains(event.position);
    if (m_HeaderHovered != hovered) {
        m_HeaderHovered = hovered;
        InvalidatePaint();
    }
}

void CollapsibleGroup::OnHoverLost() {
    if (m_HeaderHovered) {
        m_HeaderHovered = false;
        InvalidatePaint();
    }
}

void CollapsibleGroup::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        const float headerH = PropertyPanelChrome::SectionHeight();
        const Rect headerRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, headerH };
        if (headerRect.Contains(event.position)) {
            SetExpanded(!m_Expanded);
        }
    }
}

} // namespace we::runtime::kindui
