#pragma once

#include "Application/Export.hpp"

#include "Core/Widget.hpp"
#include "Widgets/Panel.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace we::UI {

// A container that manages multiple Panel widgets as tabs
class APPLICATION_API DockContainer : public Widget {
public:
    using OnTabClosed = std::function<void(const std::shared_ptr<Panel>& panel)>;
    using OnTabDragStarted = std::function<void(const std::shared_ptr<Panel>& panel, const Point& position)>;
    using OnActiveTabChanged = std::function<void(int index)>;

    DockContainer();
    virtual ~DockContainer() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void AddPanel(const std::shared_ptr<Panel>& panel);
    void RemovePanel(const std::shared_ptr<Panel>& panel);
    bool ContainsPanel(const std::shared_ptr<Panel>& panel) const;
    void FocusPanel(const std::shared_ptr<Panel>& panel);

    void SetActiveTab(int index);
    int GetActiveTab() const { return m_ActiveTabIndex; }
    int GetTabCount() const { return static_cast<int>(m_Tabs.size()); }

    void SetOnTabClosed(OnTabClosed callback) { m_OnTabClosed = std::move(callback); }
    void SetOnTabDragStarted(OnTabDragStarted callback) { m_OnTabDragStarted = std::move(callback); }
    void SetOnActiveTabChanged(OnActiveTabChanged callback) { m_OnActiveTabChanged = std::move(callback); }

private:
    struct TabInfo {
        std::shared_ptr<Panel> panel;
        Rect tabRect;
        Rect closeRect;
        bool isHovered = false;
        bool isCloseHovered = false;
        float hoverAnim = 0.0f;
    };

    float MeasureTabWidth(PaintContext& context, const TabInfo& tabInfo, bool isActive) const;
    void PaintTab(PaintContext& context, TabInfo& tabInfo, int index, float& currentX);

    std::vector<TabInfo> m_Tabs;
    int m_ActiveTabIndex = -1;

    float m_HeaderHeight = 26.0f;
    Rect m_HeaderRect;
    Rect m_ContentRect;

    int m_DragTabIndex = -1;
    Point m_DragStart{};
    bool m_TabDragCandidate = false;

    OnTabClosed m_OnTabClosed;
    OnTabDragStarted m_OnTabDragStarted;
    OnActiveTabChanged m_OnActiveTabChanged;
};

} // namespace we::UI
