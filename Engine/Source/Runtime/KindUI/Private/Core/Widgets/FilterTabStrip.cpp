// ==============================================================================
// WindEffects — KindUI — FilterTabStrip
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Widgets/FilterTabStrip.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"

#include <algorithm>

namespace we::runtime::kindui {

FilterTabStrip::FilterTabStrip() = default;
FilterTabStrip::~FilterTabStrip() = default;

void FilterTabStrip::SetTabs(std::vector<std::string> tabs) {
    if (m_TabLabels != tabs) {
        m_TabLabels = std::move(tabs);
        InvalidateLayout();
        InvalidatePaint();
    }
}

void FilterTabStrip::SetActiveTab(std::string activeTab) {
    if (m_ActiveTab != activeTab) {
        m_ActiveTab = std::move(activeTab);
        InvalidatePaint();
    }
}

void FilterTabStrip::SetOnTabSelected(std::function<void(const std::string& tab)> cb) {
    m_OnTabSelected = std::move(cb);
}

float FilterTabStrip::LayoutTabsForWidth(float originX, float originY, float availableW) {
    m_Tabs.clear();
    const float scale = (std::max)(1.0f, DPIContext::GetScale());
    const float padH = ResolveMetric(MetricToken::Space2) * scale;
    const float padV = ResolveMetric(MetricToken::Space1) * scale;
    const float tabH = PropertyPanelChrome::CategoryTabHeight();
    const float gap = ResolveMetric(MetricToken::Space1) * scale;
    const float minTabW = padH * 3.0f;

    const float leftInset = originX + padH;
    const float maxRight = availableW > 0.0f ? (originX + availableW - padH) : (leftInset + 10000.0f);

    float currentX = leftInset;
    float currentY = originY + padV;

    auto addTabSlot = [&](const std::string& label, float textWidth) {
        const float tabW = (std::max)(minTabW, textWidth + padH * 2.0f);
        if (currentX + tabW > maxRight && currentX > leftInset) {
            currentX = leftInset;
            currentY += tabH + gap;
        }
        m_Tabs.push_back(TabSlot{ label, Rect{ currentX, currentY, tabW, tabH } });
        currentX += tabW + gap;
    };

    const float captionSize = ResolveMetric(MetricToken::TextSizeCaption) * scale;
    addTabSlot("All", captionSize * 1.5f);
    for (const auto& label : m_TabLabels) {
        addTabSlot(label, static_cast<float>(label.size()) * captionSize * 0.55f);
    }

    return (currentY + tabH + padV) - originY;
}

std::string FilterTabStrip::TabAt(const Point& pos) const {
    for (const auto& tab : m_Tabs) {
        if (tab.rect.Contains(pos)) {
            return tab.label;
        }
    }
    return {};
}

Size FilterTabStrip::Measure(const Size& availableSize) {
    if (!IsVisible()) {
        m_DesiredSize = Size{ availableSize.width, 0.0f };
        return m_DesiredSize;
    }
    const float calculatedH = LayoutTabsForWidth(m_Geometry.x, m_Geometry.y, availableSize.width);
    m_DesiredSize = Size{ availableSize.width, (std::max)(calculatedH, LayoutMetrics::PropertyCategoryTabRowHeight()) };
    return m_DesiredSize;
}

void FilterTabStrip::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    LayoutTabsForWidth(allottedRect.x, allottedRect.y, allottedRect.width);
}

void FilterTabStrip::Paint(PaintContext& context) {
    if (!IsVisible()) return;
    LayoutTabsForWidth(m_Geometry.x, m_Geometry.y, m_Geometry.width);
    context.DrawRect(m_Geometry, ResolveColor(ColorToken::PanelBackground));

    for (const auto& tab : m_Tabs) {
        const bool isActive = (tab.label == "All") ? m_ActiveTab.empty() : (m_ActiveTab == tab.label);
        PropertyPanelChrome::PaintCategoryTab(context, tab.rect, tab.label, isActive, m_HoveredTab == tab.label);
    }
}

void FilterTabStrip::OnMouseMove(const MouseEvent& event) {
    if (!IsVisible()) return;
    const std::string prev = m_HoveredTab;
    m_HoveredTab = TabAt(event.position);
    if (prev != m_HoveredTab) {
        InvalidatePaint();
    }
}

void FilterTabStrip::OnMouseDown(const MouseEvent& event) {
    if (!IsVisible() || event.button != MouseButton::Left) return;
    const std::string tab = TabAt(event.position);
    if (tab.empty()) return;

    m_ActiveTab = (tab == "All") ? "" : tab;
    if (m_OnTabSelected) {
        m_OnTabSelected(m_ActiveTab);
    }
    InvalidatePaint();
}

} // namespace we::runtime::kindui
