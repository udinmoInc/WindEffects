#include "Widgets/RenderDebuggerPanel.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"

namespace we::UI {

RenderDebuggerPanel::RenderDebuggerPanel() {
    m_ScrollContentBox = std::make_shared<VerticalBox>();
    m_ControlsBox = std::make_shared<VerticalBox>();
    m_TargetsBox = std::make_shared<VerticalBox>();
    m_Scroll = std::make_shared<ScrollLayout>();
    m_ContentLabel = std::make_shared<Label>("Render debugger unavailable in foundation renderer.");
    m_Scroll->SetContent(m_ContentLabel);
}

void RenderDebuggerPanel::BuildIsolationControls() {}

void RenderDebuggerPanel::Construct() {
    AddChild(m_Scroll);
}

void RenderDebuggerPanel::ShowTargetPreview(
    const we::runtime::renderer::RenderDebuggerFrameSnapshot& /*snapshot*/, int /*targetIndex*/) {}

void RenderDebuggerPanel::RebuildTargetButtons(const we::runtime::renderer::RenderDebuggerFrameSnapshot& /*snapshot*/) {}

void RenderDebuggerPanel::UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    m_LastSnapshot = snapshot;
    m_DisplayText = "Foundation renderer active. Legacy render debugger panels are disabled.";
    if (m_ContentLabel) {
        m_ContentLabel->SetText(m_DisplayText);
    }
}

void RenderDebuggerPanel::RebuildText(const we::runtime::renderer::RenderDebuggerFrameSnapshot& /*snapshot*/) {}

Size RenderDebuggerPanel::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void RenderDebuggerPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_Scroll) {
        m_Scroll->Measure(Size{ allottedRect.width, allottedRect.height });
        m_Scroll->Arrange(allottedRect);
    }
}

void RenderDebuggerPanel::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    context.DrawRect(m_Geometry, Color{ 0.08f, 0.08f, 0.08f, 1.0f });
    if (m_Scroll) {
        m_Scroll->Paint(context);
    }
}

void RenderDebuggerPanel::OnMouseWheel(const MouseEvent& event) {
    if (m_Scroll) {
        m_Scroll->OnMouseWheel(event);
    }
}

} // namespace we::UI
