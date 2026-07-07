#pragma once

#include "Menus/Export.h"

#include "Core/Widget.h"
#include "Core/Style.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace we::UI {

// Menu item structure
struct MenuItem {
    std::string label;
    std::string shortcut;
    std::function<void()> onClick;
    bool enabled = true;
    bool checked = false;
    std::vector<std::shared_ptr<MenuItem>> submenu; // For nested menus
};

// Menu bar widget for top-level application menus
class MENUS_API MenuBar : public Widget {
public:
    MenuBar();
    virtual ~MenuBar() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    // Menu management
    void AddMenu(const std::string& label, const std::vector<std::shared_ptr<MenuItem>>& items) {
        MenuInfo menu;
        menu.label = label;
        menu.items = items;
        m_Menus.push_back(menu);
        CalculateMenuGeometries();
    }
    void RemoveMenu(const std::string& label);
    void Clear();

    // Styling
    void SetHeight(float height) { m_Height = height; }
    void SetItemSpacing(float spacing) { m_ItemSpacing = spacing; CalculateMenuGeometries(); }

private:
    struct MenuInfo {
        std::string label;
        std::vector<std::shared_ptr<MenuItem>> items;
        Rect geometry;
        bool hovered = false;
    };

    void CalculateMenuGeometries();
    MenuInfo* GetMenuAtPosition(const Point& pos);

    std::vector<MenuInfo> m_Menus;
    float m_Height = 40.0f;
    float m_ItemSpacing = 0.0f;
    float m_ItemPaddingH = 6.0f;
    int m_HoveredMenu = -1;
    bool m_MenuOpen = false;

    WidgetStyle m_Style;
    std::vector<MenuInfo> m_VisibleMenus;
    std::vector<MenuInfo> m_HiddenMenus;
    MenuInfo m_MoreMenu;
    bool m_ShowsMore = false;
};

} // namespace we::editor::menus::UI
