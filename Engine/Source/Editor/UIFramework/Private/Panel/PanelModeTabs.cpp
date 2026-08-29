#include "WindEffects/Editor/UI/Panel/PanelModeTabs.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::editor::panels {
namespace Chrome = PanelChrome;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;

void PanelModeTabs::SetTabs(std::vector<PanelModeTabDescriptor> tabs) {
    m_Tabs = std::move(tabs);
    if (m_ActiveTabId.empty() && !m_Tabs.empty()) {
        m_ActiveTabId = m_Tabs.front().id;
    }
    InvalidateLayout();
}

void PanelModeTabs::SetActiveTabId(const std::string& tabId) {
    if (m_ActiveTabId == tabId) {
        return;
    }
    m_ActiveTabId = tabId;
    InvalidatePaint();
}

Size PanelModeTabs::Measure(const Size& availableSize) {
    const float height = Chrome::ModeTabRowHeight();
    m_DesiredSize = Size{
        availableSize.width < 1.0e8f ? availableSize.width : 0.0f,
        height
    };
    return m_DesiredSize;
}

void PanelModeTabs::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    RebuildTabGeometries();
}

void PanelModeTabs::RebuildTabGeometries() {
    m_Layout.clear();
    if (m_Tabs.empty() || m_Geometry.width <= 0.0f) {
        return;
    }

    PaintContext context;
    const float padH = Chrome::TabPadH();
    const float gap = Chrome::TabGap();
    float x = m_Geometry.x + padH;

    for (const auto& tab : m_Tabs) {
        Chrome::DockTabDescriptor descriptor{};
        descriptor.title = tab.label;
        descriptor.iconName = tab.iconName;
        const bool isActive = tab.id == m_ActiveTabId;
        const float width = Chrome::MeasureDockTabWidth(context, descriptor, isActive, false);
        TabLayout layout{};
        layout.descriptor = tab;
        layout.geometry = Rect{ x, m_Geometry.y, width, m_Geometry.height };
        m_Layout.push_back(layout);
        x += width + gap;
    }
}

void PanelModeTabs::Paint(PaintContext& context) {
    Chrome::PaintToolbarRegion(context, m_Geometry);

    PaintContext measureContext;
    const float padH = Chrome::TabPadH();
    float x = m_Geometry.x + padH;

    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        const auto& tab = m_Tabs[i];
        const bool isActive = tab.id == m_ActiveTabId;
        const float hoverAnim = static_cast<int>(i) == m_HoveredIndex ? 1.0f : 0.0f;

        Chrome::DockTabDescriptor descriptor{};
        descriptor.title = tab.label;
        descriptor.iconName = tab.iconName;
        Chrome::PaintDockTab(
            context,
            descriptor,
            m_Geometry,
            x,
            isActive,
            hoverAnim,
            false,
            false);
        x += Chrome::MeasureDockTabWidth(measureContext, descriptor, isActive, false) + Chrome::TabGap();
        (void)padH;
    }
}

void PanelModeTabs::OnMouseDown(const MouseEvent& event) {
    for (size_t i = 0; i < m_Layout.size(); ++i) {
        if (!m_Layout[i].geometry.Contains(event.position)) {
            continue;
        }
        const std::string& tabId = m_Layout[i].descriptor.id;
        if (tabId != m_ActiveTabId) {
            m_ActiveTabId = tabId;
            if (m_OnTabChanged) {
                m_OnTabChanged(tabId);
            }
            InvalidatePaint();
        }
        return;
    }
}

void PanelModeTabs::OnMouseMove(const MouseEvent& event) {
    m_HoveredIndex = -1;
    for (size_t i = 0; i < m_Layout.size(); ++i) {
        if (m_Layout[i].geometry.Contains(event.position)) {
            m_HoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

} // namespace we::editor::panels
