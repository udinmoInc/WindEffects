#pragma once

#include "Core/Widget.hpp"
#include "Widgets/Panel.hpp"
#include <memory>
#include <vector>

namespace we::UI {

// A container that manages multiple Panel widgets as tabs
class DockContainer : public Widget {
public:
    DockContainer();
    virtual ~DockContainer() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void AddPanel(const std::shared_ptr<Panel>& panel);
    void RemovePanel(const std::shared_ptr<Panel>& panel);
    
    void SetActiveTab(int index);
    int GetActiveTab() const { return m_ActiveTabIndex; }

private:
    struct TabInfo {
        std::shared_ptr<Panel> panel;
        Rect tabRect;
        Rect closeRect;
        bool isHovered = false;
        bool isCloseHovered = false;
    };

    std::vector<TabInfo> m_Tabs;
    int m_ActiveTabIndex = -1;
    
    float m_HeaderHeight = 28.0f;
    Rect m_HeaderRect;
    Rect m_ContentRect;
};

} // namespace we::UI
