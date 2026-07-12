#pragma once

#include "MainFrame/Export.h"

#include "Layout/Box.h"
#include <string>
#include <functional>

namespace WindEffects::Editor::UI {

// Status bar widget for application status information
class MAINFRAME_API StatusBar : public HorizontalBox {
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

    std::shared_ptr<HorizontalBox> m_LeftBox;
    std::shared_ptr<HorizontalBox> m_RightBox;

    std::shared_ptr<class ToolButton> m_AssetsPanelButton;
    std::shared_ptr<class ToolButton> m_DiagnosticsPanelButton;
    std::shared_ptr<class CommandInput> m_CommandInput;
    std::shared_ptr<class ToolButton> m_OutputLogButton;
    std::shared_ptr<class ToolButton> m_BuildMenuButton;
    std::shared_ptr<class ToolButton> m_TraceButton;
    std::shared_ptr<class ToolButton> m_QualityMenuButton;
};

} // namespace WindEffects::Editor::UI
