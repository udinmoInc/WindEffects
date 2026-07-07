#pragma once

#include "Core/Widget.hpp"
#include "Widgets/MenuBar.hpp" // For MenuItem
#include <vector>
#include <memory>

namespace we::UI {

class DropdownMenu : public Widget {
public:
    DropdownMenu(const std::vector<std::shared_ptr<MenuItem>>& items);
    virtual ~DropdownMenu() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    // We want to handle clicks and close the menu when done

private:
    std::vector<std::shared_ptr<MenuItem>> m_Items;
    int m_HoveredItem = -1;

    int HitItemAt(const Point& pos) const;
    
    float m_ItemHeight = 24.0f;
    float m_PaddingY = 4.0f;
    float m_PaddingX = 8.0f;
};

} // namespace we::editor::menus::UI
