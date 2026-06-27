#pragma once

#include "../Core/Widget.hpp"
#include "../Core/Style.hpp"
#include "../Core/Icon.hpp"
#include <string>
#include <memory>
#include <functional>

namespace HouseEngine::UI {

// Panel widget with collapsible header and content area
class Panel : public Widget {
public:
    Panel(const std::string& title = "");
    virtual ~Panel() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;

    // Content management
    void SetContent(const std::shared_ptr<Widget>& content);
    std::shared_ptr<Widget> GetContent() const { return m_Content; }
    
    // Toolbar management
    void SetToolbar(const std::shared_ptr<Widget>& toolbar);
    std::shared_ptr<Widget> GetToolbar() const { return m_Toolbar; }

    // Header management
    void SetTitle(const std::string& title) { m_Title = title; }
    std::string GetTitle() const { return m_Title; }

    // Collapse state
    void SetExpanded(bool expanded);
    bool IsExpanded() const { return m_Expanded; }
    void Toggle() { SetExpanded(!m_Expanded); }

    // Header actions (icons on the right side of header)
    void AddHeaderAction(const std::string& iconName, std::function<void()> onClick);

    // Styling
    void SetHeaderHeight(float height) { m_HeaderHeight = height; }
    void SetCollapsible(bool collapsible) { m_Collapsible = collapsible; }

private:
    struct HeaderAction {
        std::string iconName;
        std::function<void()> onClick;
        Rect geometry;
    };

    void CalculateHeaderGeometries();
    HeaderAction* GetActionAtPosition(const Point& pos);

    std::string m_Title;
    std::shared_ptr<Widget> m_Content;
    std::shared_ptr<Widget> m_Toolbar;
    std::vector<HeaderAction> m_HeaderActions;

    bool m_Expanded = true;
    bool m_Collapsible = true;
    bool m_HeaderHovered = false;

    float m_HeaderHeight = 28.0f; // Thin header standard
    float m_ActionIconSize = 14.0f;
    float m_ActionSpacing = 4.0f;

    Rect m_HeaderRect;
    Rect m_ToolbarRect;
    Rect m_ContentRect;

    WidgetStyle m_Style;
    WidgetStyle m_HeaderStyle;
};

} // namespace HouseEngine::UI
