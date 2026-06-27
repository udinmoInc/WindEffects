#include "DropdownMenu.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Layout/OverlayManager.hpp"

namespace HouseEngine::UI {

DropdownMenu::DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
{
}

Size DropdownMenu::Measure(const Size& availableSize) {
    float maxWidth = 100.0f; // min width
    for (const auto& item : m_Items) {
        float textWidth = item->label.length() * Theme::Get().TextSizeMenu * 0.6f; // rough estimate
        if (!item->shortcut.empty()) {
            textWidth += item->shortcut.length() * Theme::Get().TextSizeMenu * 0.6f + 20.0f;
        }
        maxWidth = std::max(maxWidth, textWidth + m_PaddingX * 2.0f);
    }
    
    float height = m_PaddingY * 2.0f + m_Items.size() * m_ItemHeight;
    m_DesiredSize = Size{ maxWidth, height };
    return m_DesiredSize;
}

void DropdownMenu::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void DropdownMenu::Paint(PaintContext& context) {
    // Background and border
    context.DrawRoundedRect(m_Geometry, Theme::Get().PanelBackground, 4.0f);
    context.DrawRoundedRectOutline(m_Geometry, Theme::Get().BorderDefault, 1.0f, 4.0f);
    
    float y = m_Geometry.y + m_PaddingY;
    
    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        Rect itemRect{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, m_ItemHeight };
        
        // Hover
        if (m_HoveredItem == (int)i) {
            context.DrawRoundedRect(itemRect, Theme::Get().HoverOverlay, 2.0f);
        }
        
        // Text
        float textY = itemRect.y + (m_ItemHeight - Theme::Get().TextSizeMenu) / 2.0f;
        context.DrawText(item->label, Point{ itemRect.x + m_PaddingX, textY }, Theme::Get().TextPrimary, Theme::Get().TextSizeMenu);
        
        if (!item->shortcut.empty()) {
            float shortcutWidth = item->shortcut.length() * Theme::Get().TextSizeMenu * 0.6f;
            float shortcutX = itemRect.x + itemRect.width - m_PaddingX - shortcutWidth;
            context.DrawText(item->shortcut, Point{ shortcutX, textY }, Theme::Get().TextSecondary, Theme::Get().TextSizeMenu);
        }
        
        y += m_ItemHeight;
    }
}

void DropdownMenu::OnMouseMove(const MouseEvent& event) {
    m_HoveredItem = -1;
    float y = m_Geometry.y + m_PaddingY;
    for (size_t i = 0; i < m_Items.size(); ++i) {
        Rect itemRect{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, m_ItemHeight };
        if (itemRect.Contains(event.position)) {
            m_HoveredItem = (int)i;
            break;
        }
        y += m_ItemHeight;
    }
}

void DropdownMenu::OnMouseDown(const MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        if (m_HoveredItem >= 0 && m_HoveredItem < (int)m_Items.size()) {
            const auto& item = m_Items[m_HoveredItem];
            if (item->onClick) {
                item->onClick();
            }
        }
        
        // Close menu after click
        if (OverlayManager::Get()) {
            OverlayManager::Get()->CloseAllPopups();
        }
    }
}

} // namespace HouseEngine::UI
