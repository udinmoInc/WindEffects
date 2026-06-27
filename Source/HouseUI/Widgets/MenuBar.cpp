#include "MenuBar.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include <algorithm>

namespace HouseEngine::UI {

MenuBar::MenuBar()
    : m_Style(WidgetStyle::Panel())
{}

Size MenuBar::Measure(const Size& availableSize) {
    float totalWidth = 8.0f; // initial padding
    for (const auto& menu : m_Menus) {
        float textWidth = menu.label.length() * 13.0f * 0.6f;
        totalWidth += textWidth + 24.0f; // padding per item
    }
    return Size{ totalWidth, m_Height };
}

void MenuBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateMenuGeometries();
}

void MenuBar::Paint(PaintContext& context) {
    
    // Draw menu items
    for (size_t i = 0; i < m_Menus.size(); ++i) {
        const auto& menu = m_Menus[i];
        
        // Draw hover background
        if (menu.hovered) {
            context.DrawRoundedRect(menu.geometry, Theme::Get().HoverOverlay, 2.0f);
        }
        
        // Draw menu label
        float textX = menu.geometry.x + 12.0f;
        float textSize = Theme::Get().TextSizeBody;
        float textY = menu.geometry.y + (menu.geometry.height - textSize) / 2.0f;
        
        Color textColor = Theme::Get().TextPrimary;
        context.DrawText(menu.label, Point{ textX, textY }, textColor, textSize);
    }
}

void MenuBar::OnMouseDown(const MouseEvent& event) {
    MenuInfo* menu = GetMenuAtPosition(event.position);
    if (menu) {
        // Toggle menu open state
        m_MenuOpen = !m_MenuOpen;
        // TODO: Show menu popup
    }
}

void MenuBar::OnMouseMove(const MouseEvent& event) {
    MenuInfo* menu = GetMenuAtPosition(event.position);
    
    for (auto& m : m_Menus) {
        m.hovered = false;
    }
    
    if (menu) {
        menu->hovered = true;
    }
}

void MenuBar::AddMenu(const std::string& label, const std::vector<std::shared_ptr<MenuItem>>& items) {
    MenuInfo menu;
    menu.label = label;
    menu.items = items;
    m_Menus.push_back(menu);
    CalculateMenuGeometries();
}

void MenuBar::RemoveMenu(const std::string& label) {
    m_Menus.erase(
        std::remove_if(m_Menus.begin(), m_Menus.end(),
            [&label](const MenuInfo& m) { return m.label == label; }),
        m_Menus.end()
    );
    CalculateMenuGeometries();
}

void MenuBar::Clear() {
    m_Menus.clear();
    CalculateMenuGeometries();
}

void MenuBar::CalculateMenuGeometries() {
    float x = m_Geometry.x + 8.0f;
    
    for (auto& menu : m_Menus) {
        float textSize = Theme::Get().TextSizeBody;
        float textWidth = menu.label.length() * textSize * 0.6f;
        float width = textWidth + 24.0f; // padding
        menu.geometry = Rect{ x, m_Geometry.y, width, m_Geometry.height };
        x += width;
    }
}

MenuBar::MenuInfo* MenuBar::GetMenuAtPosition(const Point& pos) {
    for (auto& menu : m_Menus) {
        if (pos.x >= menu.geometry.x && pos.x <= menu.geometry.x + menu.geometry.width &&
            pos.y >= menu.geometry.y && pos.y <= menu.geometry.y + menu.geometry.height) {
            return &menu;
        }
    }
    return nullptr;
}

// MenuPopup implementation
MenuPopup::MenuPopup(const std::vector<std::shared_ptr<MenuItem>>& items)
    : m_Items(items)
    , m_Style(WidgetStyle::Panel())
{}

Size MenuPopup::Measure(const Size& availableSize) {
    float maxWidth = m_MinWidth;
    
    // Calculate max width from items
    for (const auto& item : m_Items) {
        float textWidth = item->label.length() * 13.0f * 0.6f;
        float shortcutWidth = item->shortcut.length() * 12.0f * 0.6f;
        float itemWidth = textWidth + shortcutWidth + 60.0f; // padding + icons
        maxWidth = std::max(maxWidth, itemWidth);
    }
    
    float totalHeight = static_cast<float>(m_Items.size()) * m_ItemHeight;
    return Size{ maxWidth, totalHeight };
}

void MenuPopup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void MenuPopup::Paint(PaintContext& context) {
    // Draw background with shadow
    context.DrawRoundedRect(m_Geometry, m_Style.background.color, m_Style.background.cornerRadius);
    context.DrawRoundedRectOutline(m_Geometry, m_Style.border.color, m_Style.border.width, m_Style.background.cornerRadius);
    
    // Draw menu items
    float y = m_Geometry.y;
    
    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        
        Rect itemRect{ m_Geometry.x, y, m_Geometry.width, m_ItemHeight };
        
        // Draw hover background
        if (static_cast<int>(i) == m_HoveredItem && item->enabled) {
            context.DrawRect(itemRect, Theme::Get().HoverOverlay);
        }
        
        // Draw checkmark if checked
        if (item->checked) {
            float checkSize = 14.0f;
            float checkX = itemRect.x + 8.0f;
            float checkY = itemRect.y + (m_ItemHeight - checkSize) / 2.0f;
            Rect checkRect{ checkX, checkY, checkSize, checkSize };
            IconPainter::DrawIcon(context, Icons::Check, checkRect, Theme::Get().SelectedAccent);
        }
        
        // Draw label
        float labelX = itemRect.x + (item->checked ? 32.0f : 12.0f);
        float labelY = itemRect.y + (m_ItemHeight - 13.0f) / 2.0f;
        
        Color textColor = item->enabled ? Theme::Get().TextPrimary : Theme::Get().TextSecondary * 0.5f;
        context.DrawText(item->label, Point{ labelX, labelY }, textColor, 13.0f);
        
        // Draw shortcut
        if (!item->shortcut.empty()) {
            float shortcutX = itemRect.x + itemRect.width - item->shortcut.length() * 12.0f * 0.6f - 12.0f;
            float shortcutY = itemRect.y + (m_ItemHeight - 12.0f) / 2.0f;
            context.DrawText(item->shortcut, Point{ shortcutX, shortcutY }, Theme::Get().TextSecondary, 12.0f);
        }
        
        // Draw submenu indicator
        if (!item->submenu.empty()) {
            float chevronSize = 12.0f;
            float chevronX = itemRect.x + itemRect.width - chevronSize - 8.0f;
            float chevronY = itemRect.y + (m_ItemHeight - chevronSize) / 2.0f;
            Rect chevronRect{ chevronX, chevronY, chevronSize, chevronSize };
            IconPainter::DrawIcon(context, Icons::ChevronRight, chevronRect, Theme::Get().TextSecondary);
        }
        
        y += m_ItemHeight;
    }
}

void MenuPopup::OnMouseDown(const MouseEvent& event) {
    MenuItem* item = GetItemAtPosition(event.position);
    if (item && item->enabled && item->onClick) {
        item->onClick();
    }
}

void MenuPopup::OnMouseMove(const MouseEvent& event) {
    m_HoveredItem = -1;
    
    MenuItem* item = GetItemAtPosition(event.position);
    if (item) {
        // Find item index
        for (size_t i = 0; i < m_Items.size(); ++i) {
            if (m_Items[i].get() == item) {
                m_HoveredItem = static_cast<int>(i);
                break;
            }
        }
    }
}

MenuItem* MenuPopup::GetItemAtPosition(const Point& pos) {
    float y = m_Geometry.y;
    
    for (const auto& item : m_Items) {
        Rect itemRect{ m_Geometry.x, y, m_Geometry.width, m_ItemHeight };
        
        if (pos.x >= itemRect.x && pos.x <= itemRect.x + itemRect.width &&
            pos.y >= itemRect.y && pos.y <= itemRect.y + itemRect.height) {
            return item.get();
        }
        
        y += m_ItemHeight;
    }
    
    return nullptr;
}

} // namespace HouseEngine::UI
