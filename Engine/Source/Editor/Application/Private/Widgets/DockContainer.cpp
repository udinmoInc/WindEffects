#include "Widgets/DockContainer.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include "Core/DockTabIconRegistry.hpp"
#include <algorithm>

namespace we::UI {

DockContainer::DockContainer() {}

void DockContainer::AddPanel(const std::shared_ptr<Panel>& panel) {
    if (!panel) return;
    // Tab strip is owned by DockContainer; suppress the panel's own header so
    // layout/hit-testing maps directly to toolbar + content.
    panel->SetHeaderHeight(0.0f);
    m_Tabs.push_back({panel, Rect{}, Rect{}, false, false});
    if (m_ActiveTabIndex == -1) {
        m_ActiveTabIndex = 0;
    }
    AddChild(panel); // This allows the container to manage the panel's lifetime in the widget tree
}

void DockContainer::RemovePanel(const std::shared_ptr<Panel>& panel) {
    auto it = std::find_if(m_Tabs.begin(), m_Tabs.end(),
        [&](const TabInfo& info) { return info.panel == panel; });
    
    if (it != m_Tabs.end()) {
        int index = (int)std::distance(m_Tabs.begin(), it);
        m_Tabs.erase(it);
        RemoveChild(panel);
        
        if (m_Tabs.empty()) {
            m_ActiveTabIndex = -1;
        } else if (m_ActiveTabIndex >= (int)m_Tabs.size()) {
            m_ActiveTabIndex = (int)m_Tabs.size() - 1;
        } else if (m_ActiveTabIndex == index) {
            // Active tab removed, switch to previous if possible
            m_ActiveTabIndex = std::max(0, m_ActiveTabIndex - 1);
        }
    }
}

void DockContainer::SetActiveTab(int index) {
    if (index >= 0 && index < (int)m_Tabs.size()) {
        m_ActiveTabIndex = index;
    }
}

Size DockContainer::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
        
        // Measure active panel's toolbar and content
        Size contentAvailable = availableSize;
        contentAvailable.height -= m_HeaderHeight;
        
        float usedHeight = m_HeaderHeight;
        
        if (auto toolbar = activePanel->GetToolbar()) {
            Size tbSize = toolbar->Measure(contentAvailable);
            contentAvailable.height -= tbSize.height;
            usedHeight += tbSize.height;
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
    
  // Arrange panel geometry so hit-testing reaches toolbar/content widgets.
    for (int i = 0; i < (int)m_Tabs.size(); ++i) {
        auto panel = m_Tabs[i].panel;
        if (i == m_ActiveTabIndex) {
            panel->Arrange(m_ContentRect);
        } else {
            panel->Arrange(Rect{0.0f, 0.0f, 0.0f, 0.0f});
        }
    }
}

void DockContainer::Paint(PaintContext& context) {
    Color panelBodyColor{0.145f, 0.145f, 0.145f, 1.0f};
    context.DrawRect(m_Geometry, panelBodyColor);
    
    // Draw header strip background
    context.DrawRect(m_HeaderRect, Theme::Get().HeaderBackground);
    
    float currentX = m_HeaderRect.x; // Tabs start flush with the left border
    
    // Paint Tabs
    for (int i = 0; i < (int)m_Tabs.size(); ++i) {
        auto& tabInfo = m_Tabs[i];
        bool isActive = (i == m_ActiveTabIndex);
        
        Color tabBg = isActive ? Color{0.1725f, 0.1725f, 0.1725f, 1.0f} : 
                     (tabInfo.isHovered ? Color{0.150f, 0.150f, 0.150f, 1.0f} : Color{0.1372f, 0.1372f, 0.1372f, 1.0f});
        
        Color tabBorder{0.227f, 0.227f, 0.227f, 1.0f};
        Color textColor = isActive ? Color{0.878f, 0.878f, 0.878f, 1.0f} : Color{0.6f, 0.6f, 0.6f, 1.0f};
        Color shadowColor{0.0f, 0.0f, 0.0f, 0.3f};
        
        float fontSize = 13.0f;
        float iconSize = 16.0f;
        float leftPadding = 12.0f;
        float rightPadding = 12.0f;
        float iconTextSpacing = 6.0f;
        float textCloseSpacing = 8.0f;
        
        std::string title = tabInfo.panel->GetTitle();
        float textWidth = context.GetTextWidth(title, fontSize);
        
        std::string panelIcon = DockTabIconRegistry::Get().GetIcon(title);
        float panelIconWidth = 0.0f;
        if (!panelIcon.empty()) {
            panelIconWidth = iconSize + iconTextSpacing;
        }
        
        float closeBtnWidth = iconSize;
        
        float tabWidth = leftPadding + panelIconWidth + textWidth + textCloseSpacing + closeBtnWidth + rightPadding;
        
        Rect tabRect{ currentX, m_HeaderRect.y, tabWidth, m_HeaderHeight };
        tabInfo.tabRect = tabRect;
        
        context.DrawRoundedRect(tabRect, tabBg, 4.0f);
        
        float flattenHeight = 4.0f;
        context.DrawRect(Rect{tabRect.x, tabRect.y + tabRect.height - flattenHeight, tabRect.width, flattenHeight}, tabBg);
        
        if (isActive) {
            context.DrawRoundedRectOutline(tabRect, tabBorder, 1.0f, 4.0f);
            context.DrawRect(Rect{tabRect.x, tabRect.y + tabRect.height - flattenHeight, 1.0f, flattenHeight}, tabBorder);
            context.DrawRect(Rect{tabRect.x + tabRect.width - 1.0f, tabRect.y + tabRect.height - flattenHeight, 1.0f, flattenHeight}, tabBorder);
            
            context.DrawRect(Rect{tabRect.x + 1.0f, tabRect.y + tabRect.height - 1.0f, tabRect.width - 2.0f, 1.0f}, tabBg);
        }
        
        float itemX = tabRect.x + leftPadding;
        if (!panelIcon.empty()) {
            float pIconY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            int codepoint = Icons::GetCodepoint(panelIcon);
            if (codepoint != 0) {
                context.DrawIcon(codepoint, Point{ itemX, pIconY }, textColor, iconSize);
            }
            itemX += panelIconWidth;
        }
        
        float titleY = m_HeaderRect.y + (m_HeaderHeight - fontSize) / 2.0f;
        float maxTextWidth = tabRect.width - (itemX - tabRect.x) - textCloseSpacing - closeBtnWidth - rightPadding;
        
        // Prevent negative or zero widths that cause rendering artifacts
        if (maxTextWidth > 0.0f) {
            context.DrawText(title, Point{ itemX, titleY }, textColor, fontSize, false);
        }
        
        if (isActive || tabInfo.isHovered) {
            float closeX = tabRect.x + tabRect.width - rightPadding - closeBtnWidth;
            float closeY = m_HeaderRect.y + (m_HeaderHeight - iconSize) / 2.0f;
            tabInfo.closeRect = Rect{ closeX, closeY, iconSize, iconSize };
            
            int crossCp = Icons::GetCodepoint(Icons::XName);
            if (crossCp != 0) {
                Color closeColor = tabInfo.isCloseHovered ? Theme::Get().TextPrimary : Theme::Get().TextSecondary;
                context.DrawIcon(crossCp, Point{ closeX, closeY }, closeColor, iconSize);
            }
        }
        
        currentX += tabWidth;
    }
    
    // Draw subtle shadow/border at bottom of entire header, except under the active tab
    Color shadowColor{0.0f, 0.0f, 0.0f, 0.3f};
    context.DrawRect(Rect{m_HeaderRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, m_HeaderRect.width, 1.0f}, shadowColor);
    
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        Rect activeTabRect = m_Tabs[m_ActiveTabIndex].tabRect;
        Color activeTabBg{0.173f, 0.173f, 0.173f, 1.0f};
        context.DrawRect(Rect{activeTabRect.x, m_HeaderRect.y + m_HeaderRect.height - 1.0f, activeTabRect.width, 1.0f}, activeTabBg);
        
        // Paint active panel content
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
        if (auto toolbar = activePanel->GetToolbar()) {
            toolbar->Paint(context);
            
            Rect tbRect = toolbar->GetGeometry();
            Rect toolbarDividerRect{ tbRect.x, tbRect.y + tbRect.height - 1.0f, tbRect.width, 1.0f };
            context.DrawRect(toolbarDividerRect, Color{0.227f, 0.227f, 0.227f, 1.0f});
        }
        if (auto content = activePanel->GetContent()) {
            content->Paint(context);
        }
    }
}

void DockContainer::OnMouseDown(const MouseEvent& event) {
    if (m_HeaderRect.Contains(event.position)) {
        for (int i = 0; i < (int)m_Tabs.size(); ++i) {
            auto& tabInfo = m_Tabs[i];
            if (tabInfo.tabRect.Contains(event.position)) {
                if ((i == m_ActiveTabIndex || tabInfo.isHovered) && tabInfo.closeRect.Contains(event.position)) {
                    // Close tab logic would go here
                    return;
                }
                
                SetActiveTab(i);
                return;
            }
        }
        return;
    }
    
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
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
    bool anyHoverChanged = false;
    
    if (m_HeaderRect.Contains(event.position)) {
        for (auto& tabInfo : m_Tabs) {
            bool wasHovered = tabInfo.isHovered;
            bool wasCloseHovered = tabInfo.isCloseHovered;
            
            tabInfo.isHovered = tabInfo.tabRect.Contains(event.position);
            tabInfo.isCloseHovered = tabInfo.isHovered && tabInfo.closeRect.Contains(event.position);
            
            if (wasHovered != tabInfo.isHovered || wasCloseHovered != tabInfo.isCloseHovered) {
                anyHoverChanged = true;
            }
        }
    } else {
        for (auto& tabInfo : m_Tabs) {
            if (tabInfo.isHovered || tabInfo.isCloseHovered) {
                tabInfo.isHovered = false;
                tabInfo.isCloseHovered = false;
                anyHoverChanged = true;
            }
        }
    }
    
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
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
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
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
    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        auto activePanel = m_Tabs[m_ActiveTabIndex].panel;
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

    if (m_ActiveTabIndex >= 0 && m_ActiveTabIndex < (int)m_Tabs.size()) {
        const auto& activePanel = m_Tabs[m_ActiveTabIndex].panel;
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

} // namespace we::UI
