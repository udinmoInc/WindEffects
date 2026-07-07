#pragma once

#include "Application/Export.hpp"
#include "Core/Widget.hpp"
#include "Layout/Box.hpp"
#include "Layout/ScrollLayout.hpp"
#include "Widgets/CheckBox.hpp"
#include "Widgets/Label.hpp"
#include "Renderer/RenderDebugger.hpp"

namespace we::UI {

class APPLICATION_API RenderDebuggerPanel : public Widget {
public:
    RenderDebuggerPanel();
    ~RenderDebuggerPanel() override = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseWheel(const MouseEvent& event) override;

    void UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);

    void Construct();

private:
    void RebuildText(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);
    void RebuildTargetButtons(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);
    void BuildIsolationControls();
    void ShowTargetPreview(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot, int targetIndex);

    std::shared_ptr<VerticalBox> m_ScrollContentBox;
    std::shared_ptr<VerticalBox> m_ControlsBox;
    std::shared_ptr<VerticalBox> m_TargetsBox;
    std::shared_ptr<ScrollLayout> m_Scroll;
    std::shared_ptr<Label> m_ContentLabel;
    std::shared_ptr<CheckBox> m_BinaryIsolationCheck;
    std::vector<std::shared_ptr<CheckBox>> m_PassChecks;
    std::vector<std::shared_ptr<class Button>> m_TargetButtons;
    std::string m_DisplayText;
    we::runtime::renderer::RenderDebuggerFrameSnapshot m_LastSnapshot{};
};

} // namespace we::UI
