#include "Widgets/RenderDiagnosticsPanel.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

namespace WindEffects::Editor::UI {

RenderDiagnosticsPanel::RenderDiagnosticsPanel() {
    m_Scroll = std::make_shared<ScrollLayout>();
    m_ContentLabel = std::make_shared<Label>("Render forensic diagnostics unavailable in foundation renderer.");
    TextStyle style;
    style.size = ThemeMetric(ThemeToken::TextSizeProperty) - 1.0f;
    style.color = ThemeColor(ThemeToken::TextSecondary);
    m_ContentLabel->SetStyle(style);
    m_Scroll->SetContent(m_ContentLabel);
}

void RenderDiagnosticsPanel::Construct() {
    AddChild(m_Scroll);
}

void RenderDiagnosticsPanel::UpdateFromReport(const we::runtime::renderer::ForensicFrameReport& report) {
    RebuildText(report);
    m_ContentLabel->SetText(m_DisplayText);
}

void RenderDiagnosticsPanel::RebuildText(const we::runtime::renderer::ForensicFrameReport& report) {
    m_DisplayText = "Foundation renderer active. Frame " + std::to_string(report.frameIndex);
    if (report.frameFailed) {
        m_DisplayText += "\nFrame failed: " + report.firstAnomalyReason;
    }
}

Size RenderDiagnosticsPanel::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void RenderDiagnosticsPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_Scroll->Measure(Size{ allottedRect.width, allottedRect.height });
    m_Scroll->Arrange(allottedRect);
}

void RenderDiagnosticsPanel::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    m_Scroll->Paint(context);
}

} // namespace WindEffects::Editor::UI
