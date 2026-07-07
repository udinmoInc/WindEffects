#include "Widgets/RenderDebuggerPanel.hpp"
#include "Widgets/RenderTargetPreviewWidget.hpp"
#include "Widgets/Button.hpp"
#include "Renderer/RenderDebugger.hpp"
#include "Renderer/RenderGpuInvestigator.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Layout/OverlayManager.hpp"
#include "Core/Theme.hpp"
#include <glm/glm.hpp>
#include <iomanip>
#include <sstream>

namespace we::UI {

namespace {

std::string F3(float v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;
    return ss.str();
}

std::string V3(const glm::vec3& v) {
    return "(" + F3(v.x) + ", " + F3(v.y) + ", " + F3(v.z) + ")";
}

void AppendMat(std::ostringstream& ss, const char* label, const glm::mat4& m, bool valid) {
    ss << label << (valid ? "" : " [INVALID]") << ":\n";
    for (int r = 0; r < 4; ++r) {
        ss << "  " << F3(m[0][r]) << " " << F3(m[1][r]) << " " << F3(m[2][r]) << " " << F3(m[3][r]) << "\n";
    }
}

const char* HealthTag(we::runtime::renderer::ForensicHealth health) {
    switch (health) {
        case we::runtime::renderer::ForensicHealth::Pass: return "[OK]";
        case we::runtime::renderer::ForensicHealth::Warning: return "[WARN]";
        case we::runtime::renderer::ForensicHealth::Error: return "[ERROR]";
        default: return "[?]";
    }
}

} // namespace

RenderDebuggerPanel::RenderDebuggerPanel() {
    m_ScrollContentBox = std::make_shared<VerticalBox>();
    m_ControlsBox = std::make_shared<VerticalBox>();
    m_TargetsBox = std::make_shared<VerticalBox>();
    m_Scroll = std::make_shared<ScrollLayout>();
    m_ContentLabel = std::make_shared<Label>("Render Debugger initializing...");
    TextStyle style;
    style.size = Theme::Get().TextSizeProperty - 1.0f;
    style.color = Theme::Get().TextSecondary;
    m_ContentLabel->SetStyle(style);
    m_ContentLabel->SetWrapText(true);
    m_ScrollContentBox->AddChild(m_ControlsBox);
    m_ScrollContentBox->AddChild(m_TargetsBox);
    m_ScrollContentBox->AddChild(m_ContentLabel);
    m_Scroll->SetContent(m_ScrollContentBox);
}

void RenderDebuggerPanel::BuildIsolationControls() {
    auto& debugger = we::runtime::renderer::RenderDebugger::Get();

    m_BinaryIsolationCheck = std::make_shared<CheckBox>("Binary Isolation Mode", debugger.IsBinaryIsolationActive());
    m_BinaryIsolationCheck->SetOnChanged([](bool checked) {
        we::runtime::renderer::RenderDebugger::Get().SetBinaryIsolationEnabled(checked);
    });
    m_ControlsBox->AddChild(m_BinaryIsolationCheck);

    const we::runtime::renderer::ForensicPassId passes[] = {
        we::runtime::renderer::ForensicPassId::Clear,
        we::runtime::renderer::ForensicPassId::AtmosphereLUT,
        we::runtime::renderer::ForensicPassId::SkyAtmosphere,
        we::runtime::renderer::ForensicPassId::Geometry,
        we::runtime::renderer::ForensicPassId::Lighting,
        we::runtime::renderer::ForensicPassId::VolumetricClouds,
        we::runtime::renderer::ForensicPassId::Fog,
        we::runtime::renderer::ForensicPassId::AerialPerspective,
        we::runtime::renderer::ForensicPassId::Grid,
        we::runtime::renderer::ForensicPassId::LuminanceReduce,
        we::runtime::renderer::ForensicPassId::Bloom,
        we::runtime::renderer::ForensicPassId::Exposure,
        we::runtime::renderer::ForensicPassId::ToneMapping,
        we::runtime::renderer::ForensicPassId::UI,
        we::runtime::renderer::ForensicPassId::Present,
    };

    for (const auto pass : passes) {
        const bool enabled = debugger.IsPassEnabled(pass);
        auto check = std::make_shared<CheckBox>(
            we::runtime::renderer::RenderForensics::PassName(pass), enabled);
        check->SetOnChanged([pass](bool checked) {
            we::runtime::renderer::RenderDebugger::Get().SetPassEnabled(pass, checked);
        });
        m_PassChecks.push_back(check);
        m_ControlsBox->AddChild(check);
    }

    auto readbackCheck = std::make_shared<CheckBox>("GPU Readback (stalls)", debugger.WantsGpuReadback());
    readbackCheck->SetOnChanged([](bool checked) {
        auto& dbg = we::runtime::renderer::RenderDebugger::Get();
        dbg.SetWantsGpuReadback(checked);
    });
    m_ControlsBox->AddChild(readbackCheck);
}

void RenderDebuggerPanel::Construct() {
    BuildIsolationControls();
    AddChild(m_Scroll);
}

void RenderDebuggerPanel::ShowTargetPreview(
    const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot,
    int targetIndex) {

    if (targetIndex < 0 || targetIndex >= static_cast<int>(snapshot.gpuInvestigation.renderTargets.size())) {
        return;
    }
    const auto& rt = snapshot.gpuInvestigation.renderTargets[static_cast<size_t>(targetIndex)];
    if (!rt.exists) return;

    auto* overlay = OverlayManager::Get();
    if (!overlay) return;

    const Size screen{ 1280.0f, 720.0f };
    auto preview = std::make_shared<RenderTargetPreviewWidget>(
        rt.displayName, rt.previewRgba, rt.previewWidth, rt.previewHeight);
    preview->Measure(screen);
    preview->Arrange(Rect{ 0.0f, 0.0f, screen.width, screen.height });
    preview->SetOnClose([overlay]() { overlay->CloseAllPopups(); });
    overlay->ShowPopup(preview, Point{ 40.0f, 40.0f });
    we::runtime::renderer::RenderGpuInvestigator::Get().SelectPreviewTarget(targetIndex);
}

void RenderDebuggerPanel::RebuildTargetButtons(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    m_TargetButtons.clear();
    m_TargetsBox->ClearChildren();

    auto header = std::make_shared<Label>("Render Targets (click to preview):");
    TextStyle style;
    style.size = Theme::Get().TextSizeProperty - 1.0f;
    style.color = Theme::Get().TextSecondary;
    header->SetStyle(style);
    m_TargetsBox->AddChild(header);

    int index = 0;
    for (const auto& rt : snapshot.gpuInvestigation.renderTargets) {
        if (!rt.exists) continue;
        const int capturedIndex = index;
        std::string label = rt.displayName;
        if (rt.stats.valid) {
            label += " avg=" + F3(rt.stats.avgR) + "," + F3(rt.stats.avgG) + "," + F3(rt.stats.avgB);
        }
        auto btn = std::make_shared<Button>(label, [this, capturedIndex]() {
            ShowTargetPreview(m_LastSnapshot, capturedIndex);
        });
        m_TargetButtons.push_back(btn);
        m_TargetsBox->AddChild(btn);
        ++index;
    }
}

void RenderDebuggerPanel::UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    m_LastSnapshot = snapshot;
    RebuildTargetButtons(snapshot);
    RebuildText(snapshot);
    m_ContentLabel->SetText(m_DisplayText);
}

void RenderDebuggerPanel::RebuildText(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    std::ostringstream ss;
    ss << "=== RENDER DEBUGGER ===\n";
    ss << "Frame: " << snapshot.frameNumber << "\n";
    ss << "FPS: " << F3(snapshot.fps) << "  Frame: " << F3(static_cast<float>(snapshot.frameTimeMs)) << " ms\n";
    ss << "CPU: " << F3(static_cast<float>(snapshot.cpuTimeMs)) << " ms  GPU: " << F3(static_cast<float>(snapshot.gpuTimeMs)) << " ms\n";
    ss << "Ctrl+Click viewport = pixel probe | F9 = baseline | Shift+F9 = compare\n";
    ss << "Probe pixel: (" << snapshot.gpuInvestigation.probePixelX << ","
       << snapshot.gpuInvestigation.probePixelY << ") UV=("
       << F3(snapshot.gpuInvestigation.probeUV.x) << "," << F3(snapshot.gpuInvestigation.probeUV.y) << ")\n\n";

    if (snapshot.firstFailure.detected) {
        ss << "!!! FIRST FAILURE !!!\n";
        ss << "Pass: " << snapshot.firstFailure.passName << "\n";
        if (!snapshot.firstFailure.shader.empty()) ss << "Shader: " << snapshot.firstFailure.shader << "\n";
        if (!snapshot.firstFailure.sourceFile.empty()) {
            ss << "Source: " << snapshot.firstFailure.sourceFile << "\n";
        }
        if (!snapshot.firstFailure.function.empty()) ss << "Function: " << snapshot.firstFailure.function << "\n";
        if (!snapshot.firstFailure.variable.empty()) ss << "Variable: " << snapshot.firstFailure.variable << "\n";
        if (!snapshot.firstFailure.expectedValue.empty()) {
            ss << "Expected: " << snapshot.firstFailure.expectedValue << "\n";
        }
        if (!snapshot.firstFailure.actualValue.empty()) {
            ss << "Actual: " << snapshot.firstFailure.actualValue << "\n";
        }
        ss << "Reason: " << snapshot.firstFailure.reason << "\n";
        if (!snapshot.firstFailure.minimalFix.empty()) {
            ss << "Recommended minimal fix: " << snapshot.firstFailure.minimalFix << "\n";
        }
        ss << "\n";
    }

    ss << "=== RENDER TARGETS ===\n";
    for (const auto& rt : snapshot.gpuInvestigation.renderTargets) {
        ss << (rt.exists ? "[OK]" : "[N/A]") << " " << rt.displayName
           << " " << rt.width << "x" << rt.height << " " << rt.format
           << " mem=" << (rt.memoryBytes / 1024) << "KB\n";
        ss << "  producer=" << rt.producerPass;
        if (!rt.consumerPasses.empty()) {
            ss << " consumers=";
            for (size_t i = 0; i < rt.consumerPasses.size(); ++i) {
                if (i > 0) ss << ",";
                ss << rt.consumerPasses[i];
            }
        }
        ss << "\n";
        if (rt.stats.valid) {
            ss << "  min=" << V3({rt.stats.minR, rt.stats.minG, rt.stats.minB})
               << " max=" << V3({rt.stats.maxR, rt.stats.maxG, rt.stats.maxB})
               << " avg=" << V3({rt.stats.avgR, rt.stats.avgG, rt.stats.avgB}) << "\n";
            ss << "  NaN=" << rt.stats.nanCount << " Inf=" << rt.stats.infCount
               << " Neg=" << rt.stats.negativeCount << "\n";
        }
    }
    ss << "\n";

    ss << "=== PASS OUTPUT VALIDATION ===\n";
    for (const auto& pass : snapshot.gpuInvestigation.passValidations) {
        ss << (pass.abnormal ? "[ABNORMAL]" : HealthTag(pass.executed ? we::runtime::renderer::ForensicHealth::Pass : we::runtime::renderer::ForensicHealth::Warning))
           << " " << pass.passName;
        if (pass.executed) ss << " CPU=" << F3(static_cast<float>(pass.cpuMs)) << "ms GPU=" << F3(static_cast<float>(pass.gpuMs)) << "ms";
        ss << "\n";
        if (!pass.inputTextures.empty()) {
            ss << "  inputs: ";
            for (size_t i = 0; i < pass.inputTextures.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << pass.inputTextures[i];
            }
            ss << "\n";
        }
        if (!pass.outputTextures.empty()) {
            ss << "  outputs: ";
            for (size_t i = 0; i < pass.outputTextures.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << pass.outputTextures[i];
            }
            ss << "\n";
        }
        if (pass.outputStats.valid) {
            ss << "  min=" << V3({pass.outputStats.minR, pass.outputStats.minG, pass.outputStats.minB})
               << " max=" << V3({pass.outputStats.maxR, pass.outputStats.maxG, pass.outputStats.maxB})
               << " avg=" << V3({pass.outputStats.avgR, pass.outputStats.avgG, pass.outputStats.avgB})
               << " lum=" << F3(pass.outputStats.avgLuminance) << "\n";
            ss << "  NaN=" << pass.outputStats.nanCount << " Inf=" << pass.outputStats.infCount
               << " Neg=" << pass.outputStats.negativeCount << "\n";
        }
        if (pass.probePixel.r != 0.0f || pass.probePixel.g != 0.0f || pass.probePixel.b != 0.0f) {
            ss << "  probeRGB=" << V3({pass.probePixel.r, pass.probePixel.g, pass.probePixel.b}) << "\n";
        }
        for (const auto& a : pass.abnormalities) ss << "  !! " << a << "\n";
    }
    ss << "\n";

    ss << "=== PIXEL INSPECTOR (probe through pipeline) ===\n";
    for (const auto& stage : snapshot.gpuInvestigation.pixelPipeline) {
        ss << stage.passName << (stage.firstInvalidStage ? " <<< FIRST INVALID" : "") << "\n";
        if (!stage.valid) { ss << "  (no data)\n"; continue; }
        ss << "  HDR=" << V3({stage.hdrRgb.r, stage.hdrRgb.g, stage.hdrRgb.b});
        if (stage.ldrRgb.r > 0.0f || stage.ldrRgb.g > 0.0f || stage.ldrRgb.b > 0.0f) {
            ss << " LDR=" << V3({stage.ldrRgb.r, stage.ldrRgb.g, stage.ldrRgb.b});
        }
        ss << "\n";
        if (stage.passId == we::runtime::renderer::ForensicPassId::SkyAtmosphere) {
            ss << "  Rayleigh=" << V3(stage.rayleighRGB) << " Mie=" << V3(stage.mieRGB)
               << " Multi=" << V3(stage.multiScatterRGB) << " Sun=" << V3(stage.sunRGB)
               << " FinalHDR=" << V3(stage.finalHdrRGB) << "\n";
        }
        if (stage.firstInvalidStage) ss << "  reason: " << stage.invalidReason << "\n";
        ss << "  |\n  v\n";
    }
    ss << "\n";

    ss << "=== RENDER GRAPH ===\n";
    for (const auto& edge : snapshot.gpuInvestigation.graphEdges) {
        ss << edge.fromPass << " --(" << edge.viaResource << ")--> " << edge.toPass << "\n";
    }
    for (const auto& node : snapshot.gpuInvestigation.graphNodes) {
        ss << "  [" << node.passName << "] exec=" << (node.executed ? "yes" : "no")
           << " gpu=" << F3(static_cast<float>(node.gpuMs)) << "ms"
           << " out=" << node.producedTarget << "\n";
    }
    ss << "\n";

    ss << "=== LUT INSPECTOR ===\n";
    for (const auto& lut : snapshot.gpuInvestigation.lutInspections) {
        ss << lut.name << " " << lut.stats.width << "x" << lut.stats.height << "\n";
        if (lut.stats.valid) {
            ss << "  min=" << V3({lut.stats.minR, lut.stats.minG, lut.stats.minB})
               << " max=" << V3({lut.stats.maxR, lut.stats.maxG, lut.stats.maxB})
               << " avg=" << V3({lut.stats.avgR, lut.stats.avgG, lut.stats.avgB}) << "\n";
            ss << "  invalidPixels=" << lut.invalidPixelCount << "\n";
            ss << "  histogram[peak]=";
            uint32_t peak = 0;
            for (uint32_t b : lut.luminanceHistogram) peak = (std::max)(peak, b);
            ss << peak << "\n";
        }
    }
    ss << "\n";

    if (snapshot.gpuInvestigation.shaderTraceValid) {
        const auto& t = snapshot.gpuInvestigation.shaderTrace;
        ss << "=== SHADER VARIABLE TRACE (SkyAtmosphere @ probe) ===\n";
        ss << "ViewDir=" << V3(t.viewDirection) << " len=" << F3(t.viewDirectionLength) << "\n";
        ss << "SunDir=" << V3(t.sunDirection) << " dot=" << F3(t.viewSunDot) << "\n";
        ss << "TransmittanceUV=" << V3({t.transmittanceUV.x, t.transmittanceUV.y, 0.0f})
           << " RGB=" << V3(t.transmittanceRGB) << "\n";
        ss << "SkyViewUV=" << V3({t.skyViewUV.x, t.skyViewUV.y, 0.0f})
           << " RGB=" << V3(t.skyViewRGB) << "\n";
        ss << "Rayleigh=" << V3(t.rayleighRGB) << " Mie=" << V3(t.mieRGB)
           << " MultiScatter=" << V3(t.multiScatterRGB) << "\n";
        ss << "SunDisk=" << V3(t.sunDiskRGB) << " FinalHDR=" << V3(t.finalHdrRGB) << "\n\n";
    }

    if (!snapshot.gpuInvestigation.frameComparisons.empty()) {
        ss << "=== FRAME CAPTURE COMPARISON ===\n";
        for (const auto& c : snapshot.gpuInvestigation.frameComparisons) {
            ss << (c.changed ? "[CHANGED] " : "[same] ") << c.label << ": " << c.before << " -> " << c.after << "\n";
        }
        ss << "\n";
    }

    if (!snapshot.validationWarnings.empty()) {
        ss << "=== VALIDATION WARNINGS ===\n";
        for (const auto& warn : snapshot.validationWarnings) {
            ss << "!! " << warn << "\n";
        }
        ss << "\n";
    }

    ss << "--- Camera ---\n";
    ss << "Position: " << V3(snapshot.camera.position) << "\n";
    ss << "Forward: " << V3(snapshot.camera.forward) << "\n";
    ss << "Right: " << V3(snapshot.camera.right) << "\n";
    ss << "Up: " << V3(snapshot.camera.up) << "\n";
    ss << "Near: " << F3(snapshot.camera.nearPlane) << "  Far: " << F3(snapshot.camera.farPlane)
       << "  FOV: " << F3(snapshot.camera.fovDegrees) << "\n";
    AppendMat(ss, "View", snapshot.camera.matrices.view, snapshot.camera.matrices.viewValid);
    AppendMat(ss, "Projection", snapshot.camera.matrices.projection, snapshot.camera.matrices.projectionValid);
    AppendMat(ss, "Inverse View", snapshot.camera.matrices.inverseView, snapshot.camera.matrices.inverseViewValid);
    AppendMat(ss, "Inverse Projection", snapshot.camera.matrices.inverseProjection, snapshot.camera.matrices.inverseProjectionValid);
    ss << "\n";

    ss << "--- Environment ---\n";
    ss << "Sun Dir: " << V3(snapshot.environment.sunDirection) << "\n";
    ss << "Sun Intensity: " << F3(snapshot.environment.sunIntensity)
       << "  Angular Radius: " << F3(snapshot.environment.sunAngularRadius) << "\n";
    ss << "Sky Intensity: " << F3(snapshot.environment.skyIntensity) << "\n";
    ss << "Planet Radius: " << F3(snapshot.environment.planetRadius)
       << "  Atmosphere Radius: " << F3(snapshot.environment.atmosphereRadius) << "\n";
    ss << "Rayleigh Scale H: " << F3(snapshot.environment.rayleighScaleHeight)
       << "  Mie Scale H: " << F3(snapshot.environment.mieScaleHeight) << "\n";
    ss << "Ozone Density: " << F3(snapshot.environment.ozoneDensity) << "\n\n";

    ss << "--- Exposure ---\n";
    ss << "Auto: " << (snapshot.exposure.autoExposure ? "on" : "off")
       << "  Manual EV: " << F3(snapshot.exposure.manualExposureEV) << "\n";
    ss << "EV100: " << F3(snapshot.exposure.ev100)
       << "  Multiplier: " << F3(snapshot.exposure.exposureMultiplier) << "\n";
    ss << "Avg Luminance: " << F3(snapshot.exposure.averageLuminance) << "\n\n";

    if (!snapshot.captureDiffs.empty()) {
        ss << "--- Legacy Capture Diff ---\n";
        if (!snapshot.captureStatus.empty()) ss << snapshot.captureStatus << "\n";
        for (const auto& diff : snapshot.captureDiffs) {
            ss << diff.resourceName << ": " << (diff.changed ? "CHANGED" : "same");
            if (diff.changed) {
                ss << " dR=" << F3(diff.maxDeltaR) << " dG=" << F3(diff.maxDeltaG) << " dB=" << F3(diff.maxDeltaB);
            }
            ss << "\n";
        }
    }

    m_DisplayText = ss.str();
}

Size RenderDebuggerPanel::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, availableSize.height };
    return m_DesiredSize;
}

void RenderDebuggerPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_Scroll->Measure(Size{ allottedRect.width, allottedRect.height });
    m_Scroll->Arrange(allottedRect);
}

void RenderDebuggerPanel::Paint(PaintContext& context) {
    m_Scroll->Paint(context);
}

void RenderDebuggerPanel::OnMouseWheel(const MouseEvent& event) {
    m_Scroll->OnMouseWheel(event);
}

} // namespace we::UI
