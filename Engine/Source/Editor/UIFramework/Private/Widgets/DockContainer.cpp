#include "Widgets/DockContainer.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include <algorithm>
#include <cmath>

namespace we::runtime::kindui {

namespace {
constexpr float kTabDragThreshold = 6.0f;
}

DockContainer::DockContainer() {
    m_HeaderHeight = ThemeMetric(MetricToken::PanelTabHeight);
}

void DockContainer::AddPanel(const std::shared_ptr<Panel>& panel) {
    if (!panel) return;
    panel->SetHeaderHeight(0.0f);
    m_Tabs.push_back({panel, Rect{}, Rect{}, false, false, 0.0f});
    if (m_ActiveTabIndex == -1) {
        m_ActiveTabIndex = 0;
    }
    AddChild(panel);
}

void DockContainer::RemovePanel(const std::shared_ptr<Panel>& panel) {
    auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(),
        [&](const TabInfo& info) { return info.panel == panel; });

    if (it != m_Tabs.end()) {
        int index = static_cast<int>(std::distance(m_Tabs.begin(), it));
        m_Tabs.erase(it);
        RemoveChild(panel);

        if (m_Tabs.empty()) {
            m_ActiveTabIndex = -1;
        } else if (m_ActiveTabIndex >= static_cast<int>(m_Tabs.size())) {
            m_ActiveTabIndex = static_cast<int>(m_Tabs.size()) - 1;
        } else if (m_ActiveTabIndex == index) {
            m_ActiveTabIndex = std::max(0, m_ActiveTabIndex - 1);
        }
    }
}

bool DockContainer::ContainsPanel(const std::shared_ptr<Panel>& panel) const {
    return std::any_of(m_Tabs.begin(), m_Tabs.end(),
        [&](const TabInfo& info) { return info.panel == panel; });
}

void DockContainer::FocusPanel(const std::shared_ptr<Panel>& panel) {
    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        if (m_Tabs[static_cast<size_t>(i)].panel == panel) {
            SetActiveTab(i);
            return;
        }
    }
}

void DockContainer::SetActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(m_Tabs.size())) {
        if (m_ActiveTabIndex != index) {
            m_ActiveTabIndex = index;
            if (m_OnActiveTabChanged) {
                m_OnActiveTabChanged(m_ActiveTabIndex);
            }
        }
    }
}

void DockContainer::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    const float speed = 12.0f;
    for (auto& tab : m_Tabs) {
        const float target = tab.isHovered ? 1.0f : 0.0f;
        tab.hoverAnim += (target - tab.hoverAnim) * std::min(1.0f, deltaTime * speed);
    }
}

Size DockContainer::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        Size contentAvailable = availableSize;
        contentAvailable.height -= m_HeaderHeight;

        float usedHeight = m_HeaderHeight;

        if (auto toolbar = activePanel->GetToolbar()) {
            if (!activePanel->IsFloatingToolbar()) {
                Size tbSize = toolbar->Measure(contentAvailable);
                contentAvailable.height -= tbSize.height;
                usedHeight += tbSize.height;
            }
        }

        if (auto content = activePanel->GetContent()) {
            Size cSize = content->Measure(contentAvailable);
            usedHeight += cSize.height;
        }

        m_DesiredSize.height = std::max(m_DesiredSize.height, usedHeight);
    }

    return m_DesiredSize;
}

void DockContainer::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    m_HeaderRect = Rect{
        allottedRect.x,
        allottedRect.y,
        allottedRect.width,
        m_HeaderHeight
    };

    m_ContentRect = Rect{
        allottedRect.x,
        allottedRect.y + m_HeaderHeight,
        allottedRect.width,
        allottedRect.height - m_HeaderHeight
    };

    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        auto panel = m_Tabs[static_cast<size_t>(i)].panel;
        if (i == m_ActiveTabIndex) {
            panel->Arrange(m_ContentRect);
        } else {
            panel->Arrange(Rect{0.0f, 0.0f, 0.0f, 0.0f});
        }
    }
}

float DockContainer::MeasureTabWidth(PaintContext& context, const TabInfo& tabInfo, bool isActive, bool flushLeft) {
    PanelChrome::DockTabDescriptor descriptor{};
    descriptor.title = tabInfo.panel->GetTitle();
    descriptor.iconName = tabInfo.panel->GetTabIcon();
    descriptor.hasBrand = tabInfo.panel->HasTabBrand();
    descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
    descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();
    const bool showClose = isActive || tabInfo.isHovered;
    return PanelChrome::MeasureDockTabWidth(context, descriptor, isActive, showClose, flushLeft);
}

void DockContainer::PaintTab(PaintContext& context, TabInfo& tabInfo, int index, float& currentX) {
    const bool isActive = (index == m_ActiveTabIndex);
    const bool showClose = isActive || tabInfo.isHovered;
    const bool flushLeft = (index == 0);

    PanelChrome::DockTabDescriptor descriptor{};
    descriptor.title = tabInfo.panel->GetTitle();
    descriptor.iconName = tabInfo.panel->GetTabIcon();
    descriptor.hasBrand = tabInfo.panel->HasTabBrand();
    descriptor.brandDescriptor = tabInfo.panel->GetTabBrandDescriptor();
    descriptor.brandLogicalSize = tabInfo.panel->GetTabBrandLogicalSize();

    const auto layout = PanelChrome::PaintDockTab(
        context,
        descriptor,
        m_HeaderRect,
        currentX,
        isActive,
        tabInfo.hoverAnim,
        showClose,
        tabInfo.isCloseHovered,
        flushLeft);

    tabInfo.tabRect = layout.tabRect;
    tabInfo.closeRect = showClose ? layout.closeRect : Rect{};
    currentX += layout.tabRect.width + PanelChrome::TabGap();
}

void DockContainer::Paint(PaintContext& context) {
    context.DrawRect(m_HeaderRect, ThemeColor(ColorToken::WorkspaceBackground));

    float currentX = m_HeaderRect.x;
    for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
        PaintTab(context, m_Tabs[static_cast<size_t>(i)], i, currentX);
    }

    const Rect bodyRect{
        m_Geometry.x,
        m_ContentRect.y,
        m_Geometry.width,
        m_Geometry.height - m_HeaderHeight
    };

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;

        if (!activePanel->IsTransparentBackground()) {
            PanelChrome::PaintPanelSurface(context, bodyRect);
        }

        if (auto content = activePanel->GetContent()) {
            context.PushClipRect(bodyRect);
            content->Paint(context);
            context.PopClipRect();
        }

        if (auto toolbar = activePanel->GetToolbar()) {
            if (!activePanel->IsFloatingToolbar()) {
                PanelChrome::PaintToolbarRegion(context, toolbar->GetGeometry());
            }
            toolbar->Paint(context);
        }
    }
}

void DockContainer::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderRect.Contains(event.position)) {
        for (int i = 0; i < static_cast<int>(m_Tabs.size()); ++i) {
            auto& tabInfo = m_Tabs[static_cast<size_t>(i)];
            if (tabInfo.tabRect.Contains(event.position)) {
                if ((i == m_ActiveTabIndex || tabInfo.isHovered) && tabInfo.closeRect.Contains(event.position)) {
                    if (m_OnTabClosed) {
                        m_OnTabClosed(tabInfo.panel);
                    }
                    return;
                }

                m_DragTabIndex = i;
                m_DragStart = event.position;
                m_TabDragCandidate = true;
                SetActiveTab(i);
                return;
            }
        }
        return;
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseDown(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseDown(event);
            }
        }
    }
}

void DockContainer::OnMouseMove(const MouseEvent& event) {
    if (m_TabDragCandidate && m_DragTabIndex >= 0) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (std::sqrt(dx * dx + dy * dy) >= kTabDragThreshold) {
            m_TabDragCandidate = false;
            if (m_OnTabDragStarted && m_DragTabIndex < static_cast<int>(m_Tabs.size())) {
                m_OnTabDragStarted(m_Tabs[static_cast<size_t>(m_DragTabIndex)].panel, event.position);
            }
            m_DragTabIndex = -1;
        }
    }

    if (m_HeaderRect.Contains(event.position)) {
        for (auto& tabInfo : m_Tabs) {
            tabInfo.isHovered = tabInfo.tabRect.Contains(event.position);
            tabInfo.isCloseHovered = tabInfo.isHovered && tabInfo.closeRect.Contains(event.position);
        }
    } else {
        for (auto& tabInfo : m_Tabs) {
            tabInfo.isHovered = false;
            tabInfo.isCloseHovered = false;
        }
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseMove(event);
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseMove(event);
            }
        }
    }
}

void DockContainer::OnMouseUp(const MouseEvent& event) {
    m_TabDragCandidate = false;
    m_DragTabIndex = -1;

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseUp(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseUp(event);
            }
        }
    }
}

void DockContainer::OnMouseWheel(const MouseEvent& event) {
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        auto activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(event.position)) {
                toolbar->OnMouseWheel(event);
                return;
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(event.position)) {
                content->OnMouseWheel(event);
            }
        }
    }
}

bool DockContainer::ShowsPointerCursor(const Point& position) const {
    if (m_HeaderRect.Contains(position)) {
        for (const auto& tabInfo : m_Tabs) {
            if (tabInfo.tabRect.Contains(position)) {
                return true;
            }
        }
    }

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < static_cast<int>(m_Tabs.size())) {
        const auto& activePanel = m_Tabs[static_cast<size_t>(m_ActiveTabIndex)].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            if (toolbar->GetGeometry().Contains(position)) {
                return toolbar->ShowsPointerCursor(position);
            }
        }
        if (auto content = activePanel->GetContent()) {
            if (content->GetGeometry().Contains(position)) {
                return content->ShowsPointerCursor(position);
            }
        }
    }

    return false;
}

} // namespace we::runtime::kindui
