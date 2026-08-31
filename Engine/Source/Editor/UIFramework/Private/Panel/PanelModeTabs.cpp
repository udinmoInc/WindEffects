#include "WindEffects/Editor/UI/Panel/PanelModeTabs.h"

#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"

#include <algorithm>

namespace we::editor::panels {
namespace Chrome = PanelChrome;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
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
    InvalidateLayout();
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

size_t PanelModeTabs::ActiveTabIndex() const {
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        if (m_Tabs[i].id == m_ActiveTabId) {
            return i;
        }
    }
    return 0;
}

void PanelModeTabs::RebuildTabGeometries() {
    m_Descriptors.clear();
    m_StripLayout.tabs.clear();
    if (m_Tabs.empty() || m_Geometry.width <= 0.0f) {
        return;
    }

    m_Descriptors.reserve(m_Tabs.size());
    for (const auto& tab : m_Tabs) {
        Chrome::DockTabDescriptor descriptor{};
        descriptor.title = tab.label;
        descriptor.icon = tab.icon;
        m_Descriptors.push_back(descriptor);
    }

    PaintContext context;
    Chrome::DockTabStripState state{};
    state.activeIndex = ActiveTabIndex();
    state.flatCorners = true;
    state.showClose = [](size_t /*index*/, bool /*isActive*/, bool /*isHovered*/) { return false; };
    m_StripLayout = Chrome::LayoutDockTabStrip(context, m_Geometry, m_Descriptors, state);
}

void PanelModeTabs::Paint(PaintContext& context) {
    if (m_Descriptors.empty() || m_StripLayout.tabs.empty()) {
        return;
    }

    Chrome::DockTabStripState state{};
    state.activeIndex = ActiveTabIndex();
    state.flatCorners = true;
    state.showClose = [](size_t /*index*/, bool /*isActive*/, bool /*isHovered*/) { return false; };
    state.hoverAnim = [this](size_t index) {
        return static_cast<int>(index) == m_HoveredIndex ? 1.0f : 0.0f;
    };
    Chrome::PaintDockTabStrip(context, m_Geometry, m_Descriptors, m_StripLayout, state);
}

void PanelModeTabs::OnMouseDown(const MouseEvent& event) {
    for (size_t i = 0; i < m_StripLayout.tabs.size(); ++i) {
        if (!m_StripLayout.tabs[i].tabRect.Contains(event.position)) {
            continue;
        }
        const std::string& tabId = m_Tabs[i].id;
        if (tabId != m_ActiveTabId) {
            m_ActiveTabId = tabId;
            if (m_OnTabChanged) {
                m_OnTabChanged(tabId);
            }
            InvalidateLayout();
        }
        return;
    }
}

void PanelModeTabs::OnMouseMove(const MouseEvent& event) {
    m_HoveredIndex = -1;
    for (size_t i = 0; i < m_StripLayout.tabs.size(); ++i) {
        if (m_StripLayout.tabs[i].tabRect.Contains(event.position)) {
            m_HoveredIndex = static_cast<int>(i);
            break;
        }
    }
}

} // namespace we::editor::panels
