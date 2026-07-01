#include "Widgets/MenuBar.hpp"
#include "Widgets/DropdownMenu.hpp"
#include "Layout/OverlayManager.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"
#include <algorithm>

namespace we::UI {

MenuBar::MenuBar()
    : m_Style(WidgetStyle::Panel())
{}

Size MenuBar::Measure(const Size& availableSize) {
    const float textSize = 13.0f;
    auto getApproxTextWidth = [textSize](const std::string& str) {
        float w = 0.0f;
        for (char c : str) {
            if (c == 'W' || c == 'M' || c == 'w' || c == 'm' || c == 'O' || c == 'Q') w += textSize * 0.70f;
            else if (c == 'i' || c == 'l' || c == 'I' || c == 't' || c == 'f' || c == 'j' || c == 'r') w += textSize * 0.28f;
            else w += textSize * 0.52f;
        }
        return w;
    };

    float totalWidth = 0.0f;
    for (size_t i = 0; i < m_Menus.size(); ++i) {
        float textWidth = getApproxTextWidth(m_Menus[i].label);
        totalWidth += textWidth + m_ItemPaddingH * 2.0f;
        if (i + 1 < m_Menus.size()) {
            totalWidth += m_ItemSpacing;
        }
    }
    m_DesiredSize = Size{ totalWidth, m_Height };
    return m_DesiredSize;
}

void MenuBar::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    CalculateMenuGeometries();
}

void MenuBar::Paint(PaintContext& context) {
    const float textSize = 13.0f;

    auto drawMenu = [&](const MenuInfo& menu, int index) {
        bool isActive = m_MenuOpen && index == m_HoveredMenu;

        if (menu.hovered && !isActive) {
            context.DrawRoundedRect(menu.geometry, Theme::Get().HoverOverlay, Theme::Get().CornerRadiusSmall);
        }

        if (isActive) {
            Rect underlineRect = menu.geometry;
            underlineRect.y += menu.geometry.height - 2.0f;
            underlineRect.height = 2.0f;
            context.DrawRect(underlineRect, Theme::Get().ActiveTabLine);
        }

        float textX = menu.geometry.x + m_ItemPaddingH;
        float textY = menu.geometry.y + (menu.geometry.height - textSize) / 2.0f;

        Color textColor = Theme::Get().TextPrimary;
        if (menu.hovered || isActive) {
            textColor = Color::White();
        }
        context.DrawText(menu.label, Point{ textX, textY }, textColor, textSize);
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

            if (wasOpen && menu->hovered) {
                m_MenuOpen = false;
            } else {
                m_MenuOpen = true;
                std::vector<std::shared_ptr<MenuItem>> itemsToShow = menu->items;

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
    if (m_MenuOpen && OverlayManager::Get() && !OverlayManager::Get()->HasOpenPopups()) {
        m_MenuOpen = false;
    }

    MenuInfo* menu = GetMenuAtPosition(event.position);

    for (auto& m : m_VisibleMenus) m.hovered = false;
    m_MoreMenu.hovered = false;

    if (menu) {
        menu->hovered = true;
    }
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
    float x = m_Geometry.x;
    float availableWidth = m_Geometry.width;
    const float textSize = 13.0f;

    m_VisibleMenus.clear();
    m_HiddenMenus.clear();
    m_ShowsMore = false;

    auto getApproxTextWidth = [textSize](const std::string& str) {
        float w = 0.0f;
        for (char c : str) {
            if (c == 'W' || c == 'M' || c == 'w' || c == 'm' || c == 'O' || c == 'Q') w += textSize * 0.70f;
            else if (c == 'i' || c == 'l' || c == 'I' || c == 't' || c == 'f' || c == 'j' || c == 'r') w += textSize * 0.28f;
            else w += textSize * 0.52f;
        }
        return w;
    };

    float moreTextW = getApproxTextWidth("More");
    float moreWidth = moreTextW + m_ItemPaddingH * 2.0f;

    for (size_t i = 0; i < m_Menus.size(); ++i) {
        auto& menu = m_Menus[i];
        float textWidth = getApproxTextWidth(menu.label);
        float itemWidth = textWidth + m_ItemPaddingH * 2.0f;

        bool isLast = (i == m_Menus.size() - 1);
        float widthNeeded = isLast ? itemWidth : (itemWidth + m_ItemSpacing + moreWidth);

        if (i >= 4 && (x - m_Geometry.x) + widthNeeded > availableWidth && !m_Menus.empty()) {
            m_ShowsMore = true;
            m_HiddenMenus.push_back(menu);
        } else {
            menu.geometry = Rect{ x, m_Geometry.y, itemWidth, m_Geometry.height };
            m_VisibleMenus.push_back(menu);
            x += itemWidth;
            if (!isLast) {
                x += m_ItemSpacing;
            }
        }
    }

    if (m_ShowsMore) {
        m_MoreMenu.label = "More";
        m_MoreMenu.geometry = Rect{ x, m_Geometry.y, moreWidth, m_Geometry.height };
        m_MoreMenu.items.clear();
        for (const auto& hidden : m_HiddenMenus) {
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

} // namespace we::editor::menus::UI
