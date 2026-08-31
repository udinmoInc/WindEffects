#pragma once

#include "Toolbar/Export.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Core/Style.h"
#include "KindUI/Core/WindIcon.h"
#include "Widgets/ToolButton.h"
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace we::editor::toolbar {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::IWidgetContext;

class ToolButton;
class ToolbarGroup;

enum class ToolbarAlignment {
    Left,
    Center,
    Right
};

enum class ToolbarGroupStyle {
  Transparent,
  ExecutionCluster
};

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
    std::shared_ptr<ToolButton> AddTool(we::runtime::kindui::WindIconRef icon, const std::string& label, std::function<void()> onClick, const std::string& tooltip = "", bool isPlayButton = false, ToolbarAlignment align = ToolbarAlignment::Left);
    void AddSeparator(ToolbarAlignment align = ToolbarAlignment::Left);
    void AddWidget(std::shared_ptr<Widget> widget, ToolbarAlignment align = ToolbarAlignment::Left);
    void AddGroup(std::shared_ptr<Widget> group, ToolbarAlignment align = ToolbarAlignment::Left);
    void Clear();

    // Active tool management
    void SetActiveTool(we::runtime::kindui::WindIconRef icon);

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
        we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
        std::shared_ptr<Widget> button;
        bool isSeparator = false;
        ToolbarAlignment align = ToolbarAlignment::Left;
    };

    std::vector<ToolInfo> m_Tools;
    we::runtime::kindui::WindIconRef m_ActiveTool = we::runtime::kindui::kWindIconNone;
    
    float m_Height = 0.0f; // resolved in constructor from ToolbarHeight token
    float m_IconSize = 0.0f;
    float m_ButtonSpacing = 0.0f;
    float m_GroupSpacing = 0.0f;
    float m_EdgePadding = 0.0f;
    float m_LeftInset = 0.0f;
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
    void SetStyle(ToolbarGroupStyle style) { m_Style = style; }
    [[nodiscard]] ToolbarGroupStyle GetStyle() const { return m_Style; }

private:
    std::vector<std::shared_ptr<Widget>> m_Items;
    bool m_Elevated = false;
    ToolbarGroupStyle m_Style = ToolbarGroupStyle::Transparent;

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

} // namespace we::editor::toolbar