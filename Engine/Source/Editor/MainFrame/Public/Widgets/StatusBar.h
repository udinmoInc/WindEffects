#pragma once

#include "MainFrame/Export.h"

#include "KindUI/Layout/Flex.h"
#include <string>
#include <functional>
#include "KindUI/Core/Style.h"

namespace we::editor::toolbar { class ToolButton; }

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::WidgetStyle;
using ::we::runtime::kindui::Row;

// Status bar widget for application status information
class MAINFRAME_API StatusBar : public Row {
public:
    StatusBar();
    ~StatusBar() override = default;

    void Construct() override;
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetHeight(float height) { m_Height = height; }
    void SetActiveFooterTab(int index);
    void SetOnFooterTabChanged(std::function<void(int)> onChanged);
    void SetOnCommandSubmitted(std::function<void(const std::string&)> onSubmitted);

    void SetOnOutputLogClicked(std::function<void()> onClicked);
    void SetOnBuildMenuClicked(std::function<void()> onClicked);
    void SetOnTraceClicked(std::function<void()> onClicked);
    void SetOnQualityMenuClicked(std::function<void()> onClicked);

private:
    void SelectPanelTab(int index, bool notify);

    float m_Height = 28.0f;
    int m_ActivePanelTab = 0;

    std::function<void(int)> m_OnFooterTabChanged;
    std::function<void()> m_OnOutputLogClicked;

    std::shared_ptr<Row> m_LeftBox;
    std::shared_ptr<Row> m_RightBox;

    std::shared_ptr<::we::editor::toolbar::ToolButton> m_AssetsPanelButton;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_DiagnosticsPanelButton;
    std::shared_ptr<class CommandInput> m_CommandInput;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_OutputLogButton;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_BuildMenuButton;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_TraceButton;
    std::shared_ptr<::we::editor::toolbar::ToolButton> m_QualityMenuButton;
};

} // namespace we::editor::shell