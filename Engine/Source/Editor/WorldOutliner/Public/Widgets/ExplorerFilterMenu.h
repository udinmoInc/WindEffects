#pragma once

#include "KindUI/Core/Widget.h"
#include "Widgets/TreeView.h"
#include <functional>
#include <string>
#include <vector>
#include "KindUI/Core/Style.h"

namespace we::editor::outliner {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


class ExplorerFilterMenu : public Widget {
public:
    using FilterOptions = TreeView::FilterOptions;
    using OnFilterChanged = std::function<void(const FilterOptions&)>;

    ExplorerFilterMenu(const FilterOptions& initialOptions, OnFilterChanged callback);
    
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

private:
    struct MenuItem {
        std::string label;
        bool* value;
        bool isSeparator = false;
        bool isRadio = false;
        int radioGroup = 0;
        Rect geometry;
    };

    void BuildMenuItems();
    int HitMenuItemIndex(const Point& position) const;

    FilterOptions m_FilterOptions;
    OnFilterChanged m_OnFilterChanged;
    std::vector<MenuItem> m_MenuItems;
    int m_HoveredItem = -1;
    int m_PressedItem = -1;
};

} // namespace we::editor::outliner