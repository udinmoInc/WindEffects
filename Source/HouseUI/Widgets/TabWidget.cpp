#include "TabWidget.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include <algorithm>

namespace HouseEngine::UI {

TabWidget::TabWidget()
    : m_TabStyle(WidgetStyle::Tab())
    , m_ActiveTabStyle(WidgetStyle::TabActive())
{}

Size TabWidget::Measure(const Size& availableSize) {
    CalculateTabGeometries();
    
    // Measure content of active tab
    if (m_ActiveTab >= 0 && m_ActiveTab < static_cast<int>(m_Tabs.size())) {
        auto& tab = m_Tabs[m_ActiveTab];
        if (tab.content) {
            Size contentSize = tab.content->Measure(availableSize);
            return Size{ availableSize.width, m_TabHeight + contentSize.height };
        }
    }
    
    return Size{ availableSize.width, m_TabHeight + 100.0f };
}

void TabWidget::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateTabGeometries();
    
    // Arrange content of active tab
    if (m_ActiveTab >= 0 && m_ActiveTab < static_cast<int>(m_Tabs.size())) {
        auto& tab = m_Tabs[m_ActiveTab];
        if (tab.content) {
            Rect contentRect{
                allottedRect.x,
                allottedRect.y + m_TabHeight,
                allottedRect.width,
                allottedRect.height - m_TabHeight
            };
            tab.content->Arrange(contentRect);
        }
    }
}

void TabWidget::Paint(PaintContext& context) {
    // Draw tab bar background
    Rect tabBarRect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, m_TabHeight };
    context.DrawRect(tabBarRect, Theme::Get().ToolbarBackground);
    
    // Draw tabs
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        const auto& tab = m_Tabs[i];
        bool isActive = (static_cast<int>(i) == m_ActiveTab);
        bool isHovered = (static_cast<int>(i) == m_HoveredTab);
        
        const WidgetStyle& style = isActive ? m_ActiveTabStyle : m_TabStyle;
        
        // Draw tab background
        Color bgColor = style.background.color;
        if (isHovered && !isActive) {
            bgColor = style.backgroundHover.color;
        }
        
        context.DrawRoundedRect(tab.geometry, bgColor, style.background.cornerRadius);
        
        // Draw tab label
        float textX = tab.geometry.x + style.padding.left;
        float textY = tab.geometry.y + (m_TabHeight - style.text.size) / 2.0f;
        
        Color textColor = style.text.color;
        if (isActive) textColor = Theme::Get().TextPrimary;
        else if (isHovered) textColor = Color::Lerp(Theme::Get().TextPrimary, Theme::Get().TextSecondary, 0.2f);
        else textColor = Theme::Get().TextSecondary;
        
        context.DrawText(tab.label, Point{ textX, textY }, textColor, style.text.size);
        
        // Draw active tab underline & glow
        if (isActive) {
            Rect underlineRect{ tab.geometry.x, tab.geometry.y + m_TabHeight - 2.0f, tab.geometry.width, 2.0f };
            context.DrawRect(underlineRect, Theme::Get().SelectedAccent);
            
            // Subtle glow (simplified as a translucent rect above underline)
            Rect glowRect{ tab.geometry.x, tab.geometry.y + m_TabHeight - 6.0f, tab.geometry.width, 4.0f };
            Color glowColor = Theme::Get().SelectedAccent;
            glowColor.a = 0.2f;
            context.DrawRect(glowRect, glowColor);
        }
    }
    
    // Draw separator line below tabs
    Rect separatorRect{
        m_Geometry.x,
        m_Geometry.y + m_TabHeight - 1.0f,
        m_Geometry.width,
        1.0f
    };
    context.DrawRect(separatorRect, Theme::Get().BorderDefault);
    
    // Draw active tab content
    if (m_ActiveTab >= 0 && m_ActiveTab < static_cast<int>(m_Tabs.size())) {
        auto& tab = m_Tabs[m_ActiveTab];
        if (tab.content) {
            tab.content->Paint(context);
        }
    }
}

void TabWidget::OnMouseDown(const MouseEvent& event) {
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        const auto& tab = m_Tabs[i];
        if (event.position.x >= tab.geometry.x && event.position.x <= tab.geometry.x + tab.geometry.width &&
            event.position.y >= tab.geometry.y && event.position.y <= tab.geometry.y + tab.geometry.height) {
            SetActiveTab(static_cast<int>(i));
            return;
        }
    }
}

void TabWidget::OnMouseMove(const MouseEvent& event) {
    m_HoveredTab = -1;
    
    for (size_t i = 0; i < m_Tabs.size(); ++i) {
        const auto& tab = m_Tabs[i];
        if (event.position.x >= tab.geometry.x && event.position.x <= tab.geometry.x + tab.geometry.width &&
            event.position.y >= tab.geometry.y && event.position.y <= tab.geometry.y + tab.geometry.height) {
            m_HoveredTab = static_cast<int>(i);
            return;
        }
    }
}

void TabWidget::AddTab(const std::string& label) {
    TabInfo tab;
    tab.label = label;
    m_Tabs.push_back(tab);
    
    if (m_ActiveTab == -1) {
        SetActiveTab(0);
    }
    
    CalculateTabGeometries();
}

void TabWidget::RemoveTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_Tabs.size())) return;
    
    m_Tabs.erase(m_Tabs.begin() + index);
    
    if (m_ActiveTab == index) {
        m_ActiveTab = std::min(m_ActiveTab, static_cast<int>(m_Tabs.size()) - 1);
    } else if (m_ActiveTab > index) {
        m_ActiveTab--;
    }
    
    CalculateTabGeometries();
}

void TabWidget::SetTabLabel(int index, const std::string& label) {
    if (index >= 0 && index < static_cast<int>(m_Tabs.size())) {
        m_Tabs[index].label = label;
        CalculateTabGeometries();
    }
}

void TabWidget::SetActiveTab(int index) {
    if (index == m_ActiveTab) return;
    if (index < 0 || index >= static_cast<int>(m_Tabs.size())) return;
    
    m_ActiveTab = index;
    
    if (m_OnTabChanged) {
        m_OnTabChanged(index);
    }
    
    // Re-arrange to show new content
    Arrange(m_Geometry);
}

void TabWidget::SetTabContent(int index, const std::shared_ptr<Widget>& content) {
    if (index >= 0 && index < static_cast<int>(m_Tabs.size())) {
        m_Tabs[index].content = content;
    }
}

std::shared_ptr<Widget> TabWidget::GetTabContent(int index) const {
    if (index >= 0 && index < static_cast<int>(m_Tabs.size())) {
        return m_Tabs[index].content;
    }
    return nullptr;
}

void TabWidget::CalculateTabGeometries() {
    float x = m_Geometry.x;
    float availableWidth = m_Geometry.width;
    int tabCount = static_cast<int>(m_Tabs.size());
    
    if (tabCount == 0) return;
    
    // Calculate tab width (equal distribution or minimum width)
    float tabWidth = std::max(m_TabMinWidth, availableWidth / tabCount);
    
    for (auto& tab : m_Tabs) {
        tab.geometry = Rect{ x, m_Geometry.y, tabWidth, m_TabHeight };
        x += tabWidth;
    }
}

} // namespace HouseEngine::UI
