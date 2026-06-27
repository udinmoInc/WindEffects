#include "Panel.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include <functional>

namespace HouseEngine::UI {

Panel::Panel(const std::string& title)
    : m_Title(title)
    , m_Style(WidgetStyle::Panel())
    , m_HeaderStyle(WidgetStyle::Panel())
{}

Size Panel::Measure(const Size& availableSize) {
    float totalHeight = m_HeaderHeight;
    
    if (m_Expanded && m_Content) {
        Size contentSize = m_Content->Measure(availableSize);
        totalHeight += contentSize.height;
    }
    
    return Size{ availableSize.width, totalHeight };
}

void Panel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    // Calculate header rect
    m_HeaderRect = Rect{
        allottedRect.x,
        allottedRect.y,
        allottedRect.width,
        m_HeaderHeight
    };
    
    // Calculate content rect
    if (m_Expanded && m_Content) {
        m_ContentRect = Rect{
            allottedRect.x,
            allottedRect.y + m_HeaderHeight,
            allottedRect.width,
            allottedRect.height - m_HeaderHeight
        };
        m_Content->Arrange(m_ContentRect);
    }
    
    CalculateHeaderGeometries();
}

void Panel::Paint(PaintContext& context) {
    // Draw panel background
    context.DrawRoundedRect(m_Geometry, m_Style.background.color, m_Style.background.cornerRadius);
    
    // Draw panel border
    context.DrawRoundedRectOutline(m_Geometry, m_Style.border.color, m_Style.border.width, m_Style.background.cornerRadius);
    
    // Draw header background
    Color headerBg = m_HeaderStyle.background.color;
    if (m_HeaderHovered && m_Collapsible) {
        headerBg = Theme::Get().HoverOverlay;
    }
    context.DrawRoundedRect(m_HeaderRect, headerBg, m_Style.background.cornerRadius);
    
    // Draw separator line below header
    Rect separatorRect{
        m_HeaderRect.x,
        m_HeaderRect.y + m_HeaderRect.height - 1.0f,
        m_HeaderRect.width,
        1.0f
    };
    context.DrawRect(separatorRect, Theme::Get().BorderDefault);
    
    // Draw collapse/expand chevron
    if (m_Collapsible) {
        float chevronSize = 12.0f; // Smaller collapse arrow
        float chevronX = m_HeaderRect.x + 6.0f; // Tighter padding
        float chevronY = m_HeaderRect.y + (m_HeaderHeight - chevronSize) / 2.0f;
        
        Rect chevronRect{ chevronX, chevronY, chevronSize, chevronSize };
        int chevronIcon = m_Expanded ? Icons::ChevronDown : Icons::ChevronRight;
        IconPainter::DrawIcon(context, chevronIcon, chevronRect, Theme::Get().TextSecondary);
    }
    
    // Draw title
    float titleX = m_HeaderRect.x + (m_Collapsible ? 22.0f : 8.0f);
    float textSize = Theme::Get().TextSizeHeader;
    float titleY = m_HeaderRect.y + (m_HeaderHeight - textSize) / 2.0f;
    context.DrawText(m_Title, Point{ titleX, titleY }, Theme::Get().TextPrimary, textSize, true);
    
    // Draw header actions
    for (const auto& action : m_HeaderActions) {
        IconPainter::DrawIcon(context, action.iconName, action.geometry, Theme::Get().TextSecondary);
    }
    
    // Draw content
    if (m_Expanded && m_Content) {
        m_Content->Paint(context);
    }
}

void Panel::OnMouseDown(const MouseEvent& event) {
    // Check if clicked on header
    if (m_HeaderRect.Contains(event.position)) {
        // Check if clicked on collapse chevron
        if (m_Collapsible) {
            float chevronSize = 14.0f;
            float chevronX = m_HeaderRect.x + 8.0f;
            float chevronY = m_HeaderRect.y + (m_HeaderHeight - chevronSize) / 2.0f;
            Rect chevronRect{ chevronX, chevronY, chevronSize, chevronSize };
            
            if (event.position.x >= chevronRect.x && event.position.x <= chevronRect.x + chevronRect.width &&
                event.position.y >= chevronRect.y && event.position.y <= chevronRect.y + chevronRect.height) {
                Toggle();
                return;
            }
        }
        
        // Check if clicked on header action
        HeaderAction* action = GetActionAtPosition(event.position);
        if (action && action->onClick) {
            action->onClick();
            return;
        }
        
        // Toggle collapse if clicked anywhere else on header
        if (m_Collapsible) {
            Toggle();
        }
    }
}

void Panel::OnMouseMove(const MouseEvent& event) {
    m_HeaderHovered = m_HeaderRect.Contains(event.position);
}

void Panel::SetContent(const std::shared_ptr<Widget>& content) {
    m_Content = content;
}

void Panel::SetExpanded(bool expanded) {
    if (m_Expanded == expanded) return;
    
    m_Expanded = expanded;
    Arrange(m_Geometry);
}

void Panel::AddHeaderAction(const std::string& iconName, std::function<void()> onClick) {
    HeaderAction action;
    action.iconName = iconName;
    action.onClick = onClick;
    m_HeaderActions.push_back(action);
    CalculateHeaderGeometries();
}

void Panel::CalculateHeaderGeometries() {
    float actionX = m_HeaderRect.x + m_HeaderRect.width - m_ActionIconSize - 8.0f;
    
    for (auto& action : m_HeaderActions) {
        float actionY = m_HeaderRect.y + (m_HeaderHeight - m_ActionIconSize) / 2.0f;
        action.geometry = Rect{ actionX, actionY, m_ActionIconSize, m_ActionIconSize };
        actionX -= m_ActionIconSize + m_ActionSpacing;
    }
}

Panel::HeaderAction* Panel::GetActionAtPosition(const Point& pos) {
    for (auto& action : m_HeaderActions) {
        if (pos.x >= action.geometry.x && pos.x <= action.geometry.x + action.geometry.width &&
            pos.y >= action.geometry.y && pos.y <= action.geometry.y + action.geometry.height) {
            return &action;
        }
    }
    return nullptr;
}

} // namespace HouseEngine::UI
