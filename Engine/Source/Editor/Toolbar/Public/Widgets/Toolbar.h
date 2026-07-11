#pragma once

#include "Toolbar/Export.h"

#include "Core/Widget.h"
#include "Layout/Box.h"
#include "Layout/Spacer.h"
#include "Core/Style.h"
#include "Core/Icon.h"
#include "Widgets/ToolButton.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace WindEffects::Editor::UI {

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
    void Clear();

    // Active tool management
    void SetActiveTool(const std::string& iconName);
    std::string GetActiveTool() const { return m_ActiveTool; }

    // Styling
    void SetHeight(float height) { m_Height = height; }
    void SetIconSize(float size) { m_IconSize = size; }
    void SetFloating(bool floating) { m_IsFloating = floating; }
    void SetLeftInset(float inset) { m_LeftInset = inset; }

private:
    struct ToolInfo {
        std::string iconName;
        std::shared_ptr<Widget> button;
        bool isSeparator = false;
        ToolbarAlignment align = ToolbarAlignment::Left;
    };

    std::vector<ToolInfo> m_Tools;
    std::string m_ActiveTool;
    
    float m_Height = 40.0f;
    float m_IconSize = 16.0f;
    float m_ButtonSpacing = 4.0f;
    float m_GroupSpacing = 10.0f;
    float m_EdgePadding = 12.0f;
    float m_LeftInset = 8.0f;    // Standard edge padding
    bool m_IsFloating = false;

    WidgetStyle m_Style;

    std::shared_ptr<Widget> HitToolAt(const Point& position) const;
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

} // namespace WindEffects::Editor::UI
