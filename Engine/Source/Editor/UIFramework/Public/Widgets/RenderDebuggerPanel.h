#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Widgets/CheckBox.h"
#include "KindUI/Widgets/Label.h"
#include "Renderer/EditorRenderDebugStub.h"

#include <memory>
#include <string>
#include <vector>

namespace we::runtime::kindui {

/// Foundation stub: legacy render debugger controls are disabled.
class UIFRAMEWORK_API RenderDebuggerPanel : public Widget {
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

    std::shared_ptr<Column> m_ScrollContentBox;
    std::shared_ptr<Column> m_ControlsBox;
    std::shared_ptr<Column> m_TargetsBox;
    std::shared_ptr<ScrollLayout> m_Scroll;
    std::shared_ptr<Label> m_ContentLabel;
    std::shared_ptr<CheckBox> m_BinaryIsolationCheck;
    std::vector<std::shared_ptr<CheckBox>> m_PassChecks;
    std::string m_DisplayText;
    we::runtime::renderer::RenderDebuggerFrameSnapshot m_LastSnapshot{};
};

} // namespace we::runtime::kindui
