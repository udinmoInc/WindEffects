#pragma once

#include "Core/Widget.hpp"
#include "Core/Style.hpp"
#include <string>
#include <vector>
#include <functional>

namespace we::UI {

// Tab widget with tabbed interface
class TabWidget : public Widget {
public:
    using OnTabChanged = std::function<void(int index)>;

    TabWidget();
    virtual ~TabWidget() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    // Tab management
    void AddTab(const std::string& label);
    void RemoveTab(int index);
    void SetTabLabel(int index, const std::string& label);
    int GetTabCount() const { return static_cast<int>(m_Tabs.size()); }
    
    // Selection
    void SetActiveTab(int index);
    int GetActiveTab() const { return m_ActiveTab; }

    // Content
    void SetTabContent(int index, const std::shared_ptr<Widget>& content);
    std::shared_ptr<Widget> GetTabContent(int index) const;

    // Callbacks
    void SetOnTabChanged(OnTabChanged callback) { m_OnTabChanged = callback; }

    // Styling
    void SetTabHeight(float height) { m_TabHeight = height; }

private:
    struct TabInfo {
        std::string label;
        std::shared_ptr<Widget> content;
        Rect geometry;
    };

    void CalculateTabGeometries();

    std::vector<TabInfo> m_Tabs;
    int m_ActiveTab = -1;
    int m_HoveredTab = -1;

    float m_TabHeight = 30.0f;
    float m_TabMinWidth = 80.0f;
    float m_TabSpacing = 0.0f;

    OnTabChanged m_OnTabChanged;

    WidgetStyle m_TabStyle;
    WidgetStyle m_ActiveTabStyle;
};

} // namespace we::editor::application::UI
