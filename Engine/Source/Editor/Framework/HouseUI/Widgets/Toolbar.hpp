#pragma once

#include "../Core/Widget.hpp"
#include "../Layout/Box.hpp"
#include "../Layout/Spacer.hpp"
#include "../Core/Style.hpp"
#include "../Core/Icon.hpp"
#include "ToolButton.hpp"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace HouseEngine::UI {

enum class ToolbarAlignment {
    Left,
    Center,
    Right
};

// Toolbar widget with icon buttons and grouping
class Toolbar : public Widget {
public:
    Toolbar();
    virtual ~Toolbar() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

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

private:
    struct ToolInfo {
        std::string iconName;
        std::shared_ptr<Widget> button;
        bool isSeparator = false;
        ToolbarAlignment align = ToolbarAlignment::Left;
    };

    std::vector<ToolInfo> m_Tools;
    std::string m_ActiveTool;
    
    float m_Height = 32.0f; // Height to approximately 32px
    float m_IconSize = 16.0f; // 16px artwork on 24x24 canvas
    float m_ButtonSpacing = 8.0f; // 8px spacing between icons within each group
    float m_GroupSpacing = 18.0f; // 18-20px spacing between groups
    bool m_IsFloating = false;

    WidgetStyle m_Style;
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

} // namespace HouseEngine::UI
