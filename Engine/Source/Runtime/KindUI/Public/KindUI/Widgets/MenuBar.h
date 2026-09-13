// ==============================================================================
// WindEffects — KindUI — MenuBar
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/WindIcon.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace we::runtime::kindui {

// Menu item structure
struct MenuItem {
    std::string label;
    std::string shortcut;
    std::string tooltip;
    WindIconRef icon = kWindIconNone;
    std::function<void()> onClick;
    bool enabled = true;
    bool checked = false;
    std::vector<std::shared_ptr<MenuItem>> submenu;
};

// Menu bar widget for top-level application menus
class KINDUI_API MenuBar : public Widget {
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

} // namespace we::runtime::kindui
