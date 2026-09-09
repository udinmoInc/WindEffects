// ==============================================================================
// WindEffects — Menus — MenuBar
// Public API surface for the Menus module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Menus/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/WindIcon.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace we::editor::menus {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


// Menu item structure
struct MenuItem {
    std::string label;
    std::string shortcut;
    std::string tooltip;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    std::function<void()> onClick;
    bool enabled = true;
    bool checked = false;
    std::vector<std::shared_ptr<MenuItem>> submenu; // For nested menus
};

// Menu bar widget for top-level application menus
class MENUS_API MenuBar : public Widget {
public:
    MenuBar();
    ~MenuBar() override;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnHoverLost() override;
    bool ShowsPointerCursor(const Point& position) const override { return m_Geometry.Contains(position); }

    // Menu management (implemented in .cpp — STL members must not cross DLL heaps)
    void AddMenu(const std::string& label, const std::vector<std::shared_ptr<MenuItem>>& items);
    void RemoveMenu(const std::string& label);
    void Clear();

    // Styling
    void SetHeight(float height) { m_Height = height; }
    void SetItemSpacing(float spacing);

private:
    struct MenuInfo {
        std::string label;
        std::vector<std::shared_ptr<MenuItem>> items;
        Rect geometry;
    };

    void CalculateMenuGeometries();
    MenuInfo* GetMenuAtPosition(const Point& pos);

    std::vector<MenuInfo> m_Menus;
    float m_Height = 0.0f;
    float m_ItemSpacing = 0.0f;
    float m_ItemPaddingH = 0.0f;
    int m_HoveredMenu = -1;
    bool m_MenuOpen = false;

    WidgetStyle m_Style;
    std::vector<MenuInfo> m_VisibleMenus;
    std::vector<MenuInfo> m_HiddenMenus;
    MenuInfo m_MoreMenu;
    bool m_ShowsMore = false;
};

} // namespace we::editor::menus
