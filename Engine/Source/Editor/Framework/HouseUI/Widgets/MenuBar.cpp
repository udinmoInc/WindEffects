#include "MenuBar.hpp"
#include "DropdownMenu.hpp"
#include "../Layout/OverlayManager.hpp"
#include "../Core/PaintContext.hpp"
#include "../Core/Theme.hpp"
#include "../Core/Icon.hpp"
#include <algorithm>

namespace HouseEngine::UI {

MenuBar::MenuBar()
    : m_Style(WidgetStyle::Panel())
{}

Size MenuBar::Measure(const Size& availableSize) {
    // The width requested is the width of all menus plus padding
    float totalWidth = 4.0f; // initial padding
    for (const auto& menu : m_Menus) {
        float textWidth = menu.label.length() * 13.0f * 0.6f;
        totalWidth += textWidth + 12.0f; // padding per item
    }
    m_DesiredSize = Size{ totalWidth, m_Height };
    return m_DesiredSize;
}

void MenuBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateMenuGeometries();
}

void MenuBar::Paint(PaintContext& context) {
    // Draw visible menu items
    auto drawMenu = [&](const MenuInfo& menu, int index) {
        bool isActive = m_MenuOpen && index == m_HoveredMenu; // Approximation of active state
        
        if (menu.hovered && !isActive) {
            context.DrawRoundedRect(menu.geometry, Theme::Get().HoverOverlay, Theme::Get().CornerRadiusSmall);
        }
        
        if (isActive) {
            Rect underlineRect = menu.geometry;
            underlineRect.y += menu.geometry.height - 2.0f;
            underlineRect.height = 2.0f;
            context.DrawRect(underlineRect, Theme::Get().ActiveTabLine);
        }
        
        float textX = menu.geometry.x + 6.0f; // 6px left padding
        float textSize = Theme::Get().TextSizeMenu;
        float textY = menu.geometry.y + (menu.geometry.height - textSize) / 2.0f;
        
        context.DrawText(menu.label, Point{ textX, textY }, Theme::Get().TextPrimary, textSize);
    };

    for (size_t i = 0; i < m_VisibleMenus.size(); ++i) {
        drawMenu(m_VisibleMenus[i], (int)i);
    }
    
    if (m_ShowsMore) {
        drawMenu(m_MoreMenu, (int)m_VisibleMenus.size());
    }
}

void MenuBar::OnMouseDown(const MouseEvent& event) {
    MenuInfo* menu = GetMenuAtPosition(event.position);
    if (menu) {
        if (OverlayManager::Get()) {
            bool wasOpen = m_MenuOpen;
            OverlayManager::Get()->CloseAllPopups();
            
            // If it was already open and we clicked the *same* hovered menu, close it.
            // If we clicked a different menu, or it wasn't open, open the new one.
            if (wasOpen && menu->hovered) {
                m_MenuOpen = false;
            } else {
                m_MenuOpen = true;
                std::vector<std::shared_ptr<MenuItem>> itemsToShow = menu->items;
                
                // Show a placeholder if empty so user knows it works
                if (itemsToShow.empty()) {
                    auto emptyItem = std::make_shared<MenuItem>();
                    emptyItem->label = "(Empty)";
                    emptyItem->enabled = false;
                    itemsToShow.push_back(emptyItem);
                }
                
                auto dropdown = std::make_shared<DropdownMenu>(itemsToShow);
                OverlayManager::Get()->ShowPopup(dropdown, Point{menu->geometry.x, menu->geometry.y + menu->geometry.height});
            }
        }
    }
}

void MenuBar::OnMouseMove(const MouseEvent& event) {
    MenuInfo* menu = GetMenuAtPosition(event.position);
    
    for (auto& m : m_VisibleMenus) m.hovered = false;
    m_MoreMenu.hovered = false;
    
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
    float x = m_Geometry.x + 4.0f; // add initial padding
    float availableWidth = m_Geometry.width;
    float textSize = Theme::Get().TextSizeMenu;
    
    m_VisibleMenus.clear();
    m_HiddenMenus.clear();
    m_ShowsMore = false;
    
    // Calculate width of "More" menu
    float moreWidth = std::string("More").length() * textSize * 0.6f + 12.0f;
    
    for (size_t i = 0; i < m_Menus.size(); ++i) {
        auto& menu = m_Menus[i];
        float textWidth = menu.label.length() * textSize * 0.6f;
        float itemWidth = textWidth + 12.0f; // 6px left and right padding inside item
        
        bool isLast = (i == m_Menus.size() - 1);
        float widthNeeded = isLast ? itemWidth : (itemWidth + moreWidth);
        
        // Never hide the first 3 items (File, Edit, View)
        if (i >= 3 && (x - m_Geometry.x) + widthNeeded > availableWidth && !m_Menus.empty()) {
            m_ShowsMore = true;
            m_HiddenMenus.push_back(menu);
        } else {
            menu.geometry = Rect{ x, m_Geometry.y, itemWidth, m_Geometry.height };
            m_VisibleMenus.push_back(menu);
            x += itemWidth;
        }
    }
    
    if (m_ShowsMore) {
        m_MoreMenu.label = "More";
        m_MoreMenu.geometry = Rect{ x, m_Geometry.y, moreWidth, m_Geometry.height };
        m_MoreMenu.items.clear();
        for (const auto& hidden : m_HiddenMenus) {
            auto topLevelItem = std::make_shared<MenuItem>();
            topLevelItem->label = hidden.label;
            topLevelItem->submenu = hidden.items;
            // Since DropdownMenu currently doesn't support submenus, we just provide the top-level items
            // If they click a top level item that has a submenu, we should ideally show another popup.
            // For now, let's flat-append them with a separator-like label if we want, or just add the submenus.
            // Wait, the DropdownMenu doesn't render submenus. Let's just flat append them with prefix for now to make them usable.
            for (const auto& item : hidden.items) {
                auto prefixedItem = std::make_shared<MenuItem>(*item);
                prefixedItem->label = hidden.label + " > " + item->label;
                m_MoreMenu.items.push_back(prefixedItem);
            }
        }
    }
}

MenuBar::MenuInfo* MenuBar::GetMenuAtPosition(const Point& pos) {
    for (auto& menu : m_VisibleMenus) {
        if (menu.geometry.Contains(pos)) {
            return &menu;
        }
    }
    if (m_ShowsMore && m_MoreMenu.geometry.Contains(pos)) {
        return &m_MoreMenu;
    }
    return nullptr;
}

} // namespace HouseEngine::UI
