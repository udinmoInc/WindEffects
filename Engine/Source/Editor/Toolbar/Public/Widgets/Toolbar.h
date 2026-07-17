#pragma once

#include "Toolbar/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/Icon.h"
#include "Widgets/ToolButton.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace we::runtime::kindui {

enum class ToolbarAlignment {
    Left,
    Center,
    Right
};

// Toolbar widget with icon buttons and grouping
class TOOLBAR_API Toolbar : public Widget {
public:
    Toolbar();
    virtual ~Toolbar() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    // Tool management
    std::shared_ptr<ToolButton> AddTool(const std::string& iconName, const std::string& label, std::function<void()> onClick, const std::string& tooltip = "", bool isPlayButton = false, ToolbarAlignment align = ToolbarAlignment::Left);
    void AddSeparator(ToolbarAlignment align = ToolbarAlignment::Left);
    void AddWidget(std::shared_ptr<Widget> widget, ToolbarAlignment align = ToolbarAlignment::Left);
    void AddGroup(std::shared_ptr<Widget> group, ToolbarAlignment align = ToolbarAlignment::Left);
    void Clear();

    // Active tool management
    void SetActiveTool(const std::string& iconName);
    std::string GetActiveTool() const { return m_ActiveTool; }

    // Styling
    void SetHeight(float height) { m_Height = height; }
    void SetIconSize(float size) { m_IconSize = size; }
    void SetFloating(bool floating) { m_IsFloating = floating; }
    void SetContext(std::shared_ptr<IWidgetContext> context);
    void SetLeftInset(float inset) { m_LeftInset = inset; }
    void SetRightInset(float inset) { m_RightInset = inset; }
    void SetEdgePadding(float padding) { m_EdgePadding = padding; }

private:
    struct ToolInfo {
        std::string iconName;
        std::shared_ptr<Widget> button;
        bool isSeparator = false;
        ToolbarAlignment align = ToolbarAlignment::Left;
    };

    std::vector<ToolInfo> m_Tools;
    std::string m_ActiveTool;
    
    float m_Height = 32.0f;
    float m_IconSize = 16.0f;
    float m_ButtonSpacing = 4.0f;
    float m_GroupSpacing = 10.0f;
    float m_EdgePadding = 8.0f;
    float m_LeftInset = 8.0f;
    float m_RightInset = 0.0f;
    bool m_IsFloating = false;

    WidgetStyle m_Style;

    std::shared_ptr<Widget> HitToolAt(const Point& position) const;
};

// Visual container for a related set of toolbar controls
class TOOLBAR_API ToolbarGroup : public Widget {
public:
    ToolbarGroup();
    ~ToolbarGroup() override = default;

    void AddChildWidget(const std::shared_ptr<Widget>& child);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void OnMouseDown(const MouseEvent& event) override;
    void OnMouseMove(const MouseEvent& event) override;
    void OnMouseUp(const MouseEvent& event) override;
    void OnMouseWheel(const MouseEvent& event) override;
    bool ShowsPointerCursor(const Point& position) const override;

    void SetElevated(bool elevated) { m_Elevated = elevated; }

private:
    std::vector<std::shared_ptr<Widget>> m_Items;
    bool m_Elevated = false;

    std::shared_ptr<Widget> HitChildAt(const Point& position) const;
};

// Separator for toolbar grouping
class ToolbarSeparator : public Widget {
public:
    ToolbarSeparator();
    virtual ~ToolbarSeparator() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
};

} // namespace we::runtime::kindui
