#pragma once

#include "../Core/Widget.hpp"
#include "../Core/Style.hpp"
#include <string>
#include <functional>
#include <memory>

namespace HouseEngine::UI {

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
class MenuBar : public Widget {
public:
    MenuBar();
    virtual ~MenuBar() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    // Menu management
    void AddMenu(const std::string& label, const std::vector<std::shared_ptr<MenuItem>>& items);
    void RemoveMenu(const std::string& label);
    void Clear();

    // Styling
    void SetHeight(float height) { m_Height = height; }

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
    float m_Height = 34.0f;
    int m_HoveredMenu = -1;
    bool m_MenuOpen = false;

    WidgetStyle m_Style;
};

// Menu popup widget (dropdown menu)
class MenuPopup : public Widget {
public:
    MenuPopup(const std::vector<std::shared_ptr<MenuItem>>& items);
    virtual ~MenuPopup() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    void SetPosition(const Point& pos) { m_Position = pos; }

private:
    MenuItem* GetItemAtPosition(const Point& pos);

    std::vector<std::shared_ptr<MenuItem>> m_Items;
    Point m_Position;
    float m_ItemHeight = 28.0f;
    float m_MinWidth = 180.0f;
    int m_HoveredItem = -1;

    WidgetStyle m_Style;
};

} // namespace HouseEngine::UI
