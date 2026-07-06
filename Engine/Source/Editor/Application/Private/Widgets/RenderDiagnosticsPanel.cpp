#include "Widgets/RenderDiagnosticsPanel.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Core/Theme.hpp"
#include <iomanip>
#include <sstream>

namespace we::UI {

namespace {

const char* HealthGlyph(we::runtime::renderer::ForensicHealth health) {
    switch (health) {
        case we::runtime::renderer::ForensicHealth::Pass: return "\xF0\x9F\x9F\xA2"; // green circle
        case we::runtime::renderer::ForensicHealth::Warning: return "\xF0\x9F\x9F\xA1"; // yellow
        case we::runtime::renderer::ForensicHealth::Error: return "\xF0\x9F\x94\xB4"; // red
        default: return "?";
    }
}

std::string F3(float v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;
    return ss.str();
}

} // namespace

RenderDiagnosticsPanel::RenderDiagnosticsPanel() {
    m_Scroll = std::make_shared<ScrollLayout>();
    m_ContentLabel = std::make_shared<Label>("Render forensic diagnostics initializing...");
    TextStyle style;
    style.size = Theme::Get().TextSizeProperty - 1.0f;
    style.color = Theme::Get().TextSecondary;
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
    std::ostringstream ss;
    ss << "Render Forensics — Frame " << report.frameIndex << "\n";
    if (report.frameFailed) {
        ss << HealthGlyph(we::runtime::renderer::ForensicHealth::Error)
           << " FRAME FAILED: " << we::runtime::renderer::RenderForensics::PassName(report.firstAnomalyPass)
           << "\n  " << report.firstAnomalyReason << "\n";
        if (!report.rootCauseFile.empty()) {
            ss << "  Source: " << report.rootCauseFile << ":" << report.rootCauseLine << "\n";
        }
        if (!report.minimalFix.empty()) {
            ss << "  Fix: " << report.minimalFix << "\n";
        }
    }
    if (!report.conclusion.executiveSummary.empty()) {
        ss << "\nConclusion: " << report.conclusion.executiveSummary << "\n";
        if (report.conclusion.duplicateExposureIsInstrumentationArtifact) {
            ss << "  Duplicate exposure: instrumentation artifact (not GPU double-dispatch)\n";
        }
        if (report.conclusion.firstInvalidOutputPass != we::runtime::renderer::ForensicPassId::Count) {
            ss << "  First invalid output: "
               << we::runtime::renderer::RenderForensics::PassName(report.conclusion.firstInvalidOutputPass)
               << " — " << report.conclusion.firstInvalidOutputReason << "\n";
        }
    }
    ss << "\nCamera: (" << F3(report.cameraLog.cameraX) << ", "
       << F3(report.cameraLog.cameraY) << ", " << F3(report.cameraLog.cameraZ) << ")\n";
    ss << "Exposure EV: " << F3(report.cameraLog.exposureEV)
       << "  Multiplier: " << F3(report.cameraLog.exposureMultiplier)
       << "  EV100: " << F3(report.cameraLog.ev100)
       << "  AvgLum: " << F3(report.cameraLog.avgSceneLuminance) << "\n";
    ss << "Auto Exposure: " << (report.cameraLog.autoExposureEnabled ? "on" : "off")
       << "  Manual EV: " << F3(report.cameraLog.manualExposureEV)
       << "  Sun-derived EV: " << F3(report.cameraLog.sunDerivedExposureEV) << "\n";
    ss << "Sun: (" << F3(report.cameraLog.sunDirX) << ","
       << F3(report.cameraLog.sunDirY) << "," << F3(report.cameraLog.sunDirZ) << ")  I="
       << F3(report.cameraLog.sunIntensity) << "\n\n";

    ss << "Passes (" << report.passes.size() << "):\n";
    for (const auto& pass : report.passes) {
        ss << HealthGlyph(pass.health) << " " << pass.globalOrder << ". " << pass.passName;
        if (pass.executionNumber > 1) ss << " (#" << pass.executionNumber << ")";
        if (pass.cpuMs > 0.0) ss << "  " << F3(static_cast<float>(pass.cpuMs)) << "ms";
        ss << "\n";
        if (pass.delta.valid) {
            ss << "   delta avg: " << F3(pass.delta.avgRBefore) << "," << F3(pass.delta.avgGBefore) << "," << F3(pass.delta.avgBBefore)
               << " -> " << F3(pass.delta.avgRAfter) << "," << F3(pass.delta.avgGAfter) << "," << F3(pass.delta.avgBAfter) << "\n";
        }
        if (pass.outputStats.valid) {
            const auto& s = pass.outputStats;
            ss << "   min=(" << F3(s.minR) << "," << F3(s.minG) << "," << F3(s.minB) << ")"
               << " max=(" << F3(s.maxR) << "," << F3(s.maxG) << "," << F3(s.maxB) << ")"
               << " avg=(" << F3(s.avgR) << "," << F3(s.avgG) << "," << F3(s.avgB) << ")\n";
            ss << "   lum=" << F3(s.avgLuminance)
               << " white=" << F3(s.whitePixelPercent) << "%"
               << " black=" << F3(s.blackPixelPercent) << "%\n";
            ss << "   center=(" << F3(s.corners.center.r) << ","
               << F3(s.corners.center.g) << "," << F3(s.corners.center.b) << ")\n";
        }
        for (const auto& err : pass.validationErrors) {
            ss << "   ! " << err << "\n";
        }
        ss << "\n";
    }
    m_DisplayText = ss.str();
}

Size RenderDiagnosticsPanel::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, availableSize.height };
    return m_DesiredSize;
}

void RenderDiagnosticsPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_Scroll->Measure(Size{ allottedRect.width, allottedRect.height });
    m_Scroll->Arrange(allottedRect);
}

void RenderDiagnosticsPanel::Paint(PaintContext& context) {
    m_Scroll->Paint(context);
}

} // namespace we::UI
