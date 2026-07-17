#pragma once

#include "KindUI/Core/Widget.h"
#include "Widgets/MenuBar.h" // For MenuItem
#include <vector>
#include <memory>

namespace we::runtime::kindui {

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
    
    float m_ItemHeight = 22.0f;
    float m_PaddingY = 3.0f;
    float m_PaddingX = 6.0f;
};

} // namespace we::editor::menus::UI
