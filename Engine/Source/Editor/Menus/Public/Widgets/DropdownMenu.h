#pragma once

#include "KindUI/Core/Widget.h"
#include "Widgets/MenuBar.h" // For MenuItem
#include <vector>
#include <memory>
#include "KindUI/Core/Style.h"

namespace we::editor::menus {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;


class MENUS_API DropdownMenu : public Widget {
public:
    DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items);
    virtual ~DropdownMenu() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;

private:
    std::vector<std::shared_ptr<MenuItem>> m_Items;
    int m_HoveredItem = -1;
    float m_ScrollOffset = 0.0f;

    int HitItemAt(const Point& pos) const;
    
    float m_ItemHeight = 24.0f;
    float m_PaddingY = 4.0f;
    float m_PaddingX = 8.0f;
};

} // namespace we::editor::menus
