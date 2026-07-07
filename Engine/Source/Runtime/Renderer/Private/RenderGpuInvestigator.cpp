#include "Renderer/RenderGpuInvestigator.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

std::string FmtRgb(float r, float g, float b) {
    std::ostringstream ss;
    ss << "(" << r << "," << g << "," << b << ")";
    return ss.str();
}

uint64_t EstimateMemoryBytes(uint32_t width, uint32_t height, const std::string& format) {
    uint32_t bpp = 8;
    if (format.find("D32") != std::string::npos) bpp = 4;
    else if (format.find("R32G32B32A32") != std::string::npos) bpp = 16;
    else if (format.find("R16G16B16A16") != std::string::npos) bpp = 8;
    return static_cast<uint64_t>(width) * height * bpp;
}

std::vector<std::string> PassInputTextures(ForensicPassId pass) {
    switch (pass) {
        case ForensicPassId::Clear: return {};
        case ForensicPassId::AtmosphereLUT: return {"SceneEnvironment"};
        case ForensicPassId::SkyAtmosphere: return {"TransmittanceLUT", "SkyViewLUT", "MultiScatterLUT"};
        case ForensicPassId::Geometry: return {"OffscreenColor", "OffscreenDepth"};
        case ForensicPassId::Lighting: return {"OffscreenColor", "OffscreenDepth"};
        case ForensicPassId::VolumetricClouds: return {"OffscreenColor", "CloudHistory"};
        case ForensicPassId::Fog: return {"OffscreenColor", "OffscreenDepth", "AerialLUT"};
        case ForensicPassId::LuminanceReduce: return {"SceneHDR"};
        case ForensicPassId::Bloom: return {"SceneHDR", "BloomA"};
        case ForensicPassId::Exposure: return {"SceneHDR", "LuminanceAvg"};
        case ForensicPassId::ToneMapping: return {"SceneHDR"};
        case ForensicPassId::UI: return {"SceneHDR", "Swapchain"};
        default: return {"OffscreenColor"};
    }
}

std::vector<std::string> PassOutputTextures(ForensicPassId pass) {
    switch (pass) {
        case ForensicPassId::AtmosphereLUT:
            return {"TransmittanceLUT", "SkyViewLUT", "MultiScatterLUT", "AerialLUT"};
        case ForensicPassId::LuminanceReduce: return {"LuminanceTiles"};
        case ForensicPassId::Bloom: return {"BloomA", "BloomB"};
        case ForensicPassId::ToneMapping: return {"SceneLDR"};
        case ForensicPassId::UI: return {"Swapchain"};
        case ForensicPassId::Present: return {"Swapchain"};
        default: return {"OffscreenColor"};
    }
}

} // namespace

RenderGpuInvestigator& RenderGpuInvestigator::Get() {
    static RenderGpuInvestigator instance;
    return instance;
}

void RenderGpuInvestigator::SetProbeFromViewportUV(float u, float v, uint32_t bufferWidth, uint32_t bufferHeight) {
    m_ProbeUV = glm::clamp(glm::vec2(u, v), glm::vec2(0.0f), glm::vec2(1.0f));
    if (bufferWidth == 0 || bufferHeight == 0) return;
    m_ProbePixelX = static_cast<uint32_t>(m_ProbeUV.x * static_cast<float>(bufferWidth - 1));
    m_ProbePixelY = static_cast<uint32_t>(m_ProbeUV.y * static_cast<float>(bufferHeight - 1));
}

void RenderGpuInvestigator::SetProbePixel(uint32_t x, uint32_t y, uint32_t bufferWidth, uint32_t bufferHeight) {
    if (bufferWidth == 0 || bufferHeight == 0) return;
    m_ProbePixelX = (std::min)(x, bufferWidth - 1);
    m_ProbePixelY = (std::min)(y, bufferHeight - 1);
    m_ProbeUV.x = static_cast<float>(m_ProbePixelX) / static_cast<float>(bufferWidth - 1);
    m_ProbeUV.y = static_cast<float>(m_ProbePixelY) / static_cast<float>(bufferHeight - 1);
}

void RenderGpuInvestigator::RequestBaselineCapture() {
    m_RequestBaseline = true;
}

void RenderGpuInvestigator::RequestComparisonCapture() {
    m_RequestComparison = true;
}

std::vector<uint8_t> RenderGpuInvestigator::BuildDownscaledPreview(
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height,
    uint32_t maxDim,
    uint32_t& outWidth,
    uint32_t& outHeight) {

    std::vector<uint8_t> preview;
    if (rgba.empty() || width == 0 || height == 0) {
        outWidth = outHeight = 0;
        return preview;
    }

    const uint32_t scale = (std::max)(1u, (std::max)(width, height) / (std::max)(1u, maxDim));
    outWidth = (std::max)(1u, width / scale);
    outHeight = (std::max)(1u, height / scale);
    preview.resize(static_cast<size_t>(outWidth) * outHeight * 4);

    for (uint32_t y = 0; y < outHeight; ++y) {
        for (uint32_t x = 0; x < outWidth; ++x) {
            const uint32_t sx = (std::min)(x * scale, width - 1);
            const uint32_t sy = (std::min)(y * scale, height - 1);
            const size_t src = (static_cast<size_t>(sy) * width + sx) * 4;
            const size_t dst = (static_cast<size_t>(y) * outWidth + x) * 4;
            if (src + 2 >= rgba.size()) continue;
            auto toByte = [](float v) -> uint8_t {
                if (!std::isfinite(v)) return 255;
                v = std::clamp(v, 0.0f, 1.0f);
                return static_cast<uint8_t>(v * 255.0f);
            };
            const float lum = 0.2126f * rgba[src] + 0.7152f * rgba[src + 1] + 0.0722f * rgba[src + 2];
            const bool hdr = lum > 1.0f;
            if (hdr) {
                const float t = std::clamp(lum / 64.0f, 0.0f, 1.0f);
                preview[dst] = static_cast<uint8_t>(t * 255.0f);
                preview[dst + 1] = static_cast<uint8_t>((1.0f - t) * 128.0f);
                preview[dst + 2] = 255;
            } else {
                preview[dst] = toByte(rgba[src]);
                preview[dst + 1] = toByte(rgba[src + 1]);
                preview[dst + 2] = toByte(rgba[src + 2]);
            }
            preview[dst + 3] = 255;
        }
    }
    return preview;
}

std::array<uint32_t, 64> RenderGpuInvestigator::BuildLuminanceHistogram(
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height) {

    std::array<uint32_t, 64> hist{};
    if (rgba.empty() || width == 0 || height == 0) return hist;

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
            if (idx + 2 >= rgba.size()) continue;
            const float lum = 0.2126f * rgba[idx] + 0.7152f * rgba[idx + 1] + 0.0722f * rgba[idx + 2];
            if (!std::isfinite(lum)) continue;
            const float t = std::clamp(std::log2(std::max(lum, 1e-6f)) + 10.0f, 0.0f, 63.999f);
            ++hist[static_cast<size_t>(t)];
        }
    }
    return hist;
}

bool RenderGpuInvestigator::IsPixelInvalid(const ForensicPixelRGBA& px, bool isLdr, float maxHdr) const {
    if (std::isnan(px.r) || std::isnan(px.g) || std::isnan(px.b)) return true;
    if (std::isinf(px.r) || std::isinf(px.g) || std::isinf(px.b)) return true;
    if (px.r < 0.0f || px.g < 0.0f || px.b < 0.0f) return true;
    if (isLdr) {
        return px.r > 1.05f || px.g > 1.05f || px.b > 1.05f;
    }
    return px.r > maxHdr || px.g > maxHdr || px.b > maxHdr;
}

void RenderGpuInvestigator::MergeCatalogIntoTargets(
    GpuInvestigationReport& report,
    const std::vector<GpuResourceCatalogEntry>& catalog,
    const ForensicFrameReport& forensicReport) const {

    report.renderTargets.clear();
    for (const auto& entry : catalog) {
        GpuRenderTargetInfo rt{};
        rt.id = entry.id;
        rt.displayName = entry.displayName;
        rt.width = entry.width;
        rt.height = entry.height;
        rt.format = entry.format;
        rt.memoryBytes = entry.memoryBytes ? entry.memoryBytes
            : EstimateMemoryBytes(entry.width, entry.height, entry.format);
        rt.producerPass = entry.producerPass;
        rt.consumerPasses = entry.consumerPasses;
        rt.exists = entry.exists;

        for (const auto& pass : forensicReport.passes) {
            if (!pass.outputStats.valid) continue;
            if (pass.metadata.outputTarget.find(entry.displayName) != std::string::npos
                || pass.passName == entry.producerPass) {
                rt.stats = pass.outputStats;
            }
        }
        report.renderTargets.push_back(std::move(rt));
    }
}

void RenderGpuInvestigator::ValidatePasses(GpuInvestigationReport& report, float maxHdrComponent) const {
    for (auto& pass : report.passValidations) {
        pass.abnormal = false;
        pass.abnormalities.clear();
        if (!pass.executed) continue;

        const bool isLdr = RenderForensics::IsLdrPass(pass.passId);
        const auto& s = pass.outputStats;
        if (s.valid) {
            if (s.nanCount > 0) {
                pass.abnormal = true;
                pass.abnormalities.push_back("NaN in output (" + std::to_string(s.nanCount) + " samples)");
            }
            if (s.infCount > 0) {
                pass.abnormal = true;
                pass.abnormalities.push_back("Inf in output (" + std::to_string(s.infCount) + " samples)");
            }
            if (s.negativeCount > 0) {
                pass.abnormal = true;
                pass.abnormalities.push_back("Negative RGB (" + std::to_string(s.negativeCount) + " samples)");
            }
            if (!isLdr && (s.maxR > maxHdrComponent || s.maxG > maxHdrComponent || s.maxB > maxHdrComponent)) {
                pass.abnormal = true;
                pass.abnormalities.push_back("HDR exceeds physical limit " + std::to_string(maxHdrComponent)
                    + " max=" + FmtRgb(s.maxR, s.maxG, s.maxB));
            }
            if (isLdr && (s.maxR > 1.05f || s.maxG > 1.05f || s.maxB > 1.05f)) {
                pass.abnormal = true;
                pass.abnormalities.push_back("LDR still HDR after tonemap max=" + FmtRgb(s.maxR, s.maxG, s.maxB));
            }
        }

        if (pass.probePixel.r != 0.0f || pass.probePixel.g != 0.0f || pass.probePixel.b != 0.0f || pass.outputStats.valid) {
            if (IsPixelInvalid(pass.probePixel, isLdr, maxHdrComponent)) {
                pass.abnormal = true;
                pass.abnormalities.push_back("Probe pixel invalid at (" + std::to_string(report.probePixelX)
                    + "," + std::to_string(report.probePixelY) + ")="
                    + FmtRgb(pass.probePixel.r, pass.probePixel.g, pass.probePixel.b));
            }
        }
    }
}

void RenderGpuInvestigator::DetectFirstFailure(
    GpuInvestigationReport& report,
    const ForensicFrameReport& forensicReport) const {

    report.firstFailure = {};
    report.validationWarnings.clear();

    if (forensicReport.frameFailed || forensicReport.conclusion.firstInvalidOutputPass != ForensicPassId::Count) {
        report.firstFailure.detected = true;
        const ForensicPassId failPass = forensicReport.firstAnomalyPass != ForensicPassId::Count
            ? forensicReport.firstAnomalyPass
            : forensicReport.conclusion.firstInvalidOutputPass;
        report.firstFailure.pass = static_cast<int>(failPass);
        report.firstFailure.passName = RenderForensics::PassName(failPass);
        report.firstFailure.reason = !forensicReport.firstAnomalyReason.empty()
            ? forensicReport.firstAnomalyReason
            : forensicReport.conclusion.firstInvalidOutputReason;
        report.firstFailure.sourceFile = !forensicReport.rootCauseFile.empty()
            ? forensicReport.rootCauseFile
            : forensicReport.conclusion.rootCauseFile;
        report.firstFailure.function = forensicReport.conclusion.rootCauseFunction;
        report.firstFailure.minimalFix = !forensicReport.minimalFix.empty()
            ? forensicReport.minimalFix
            : forensicReport.conclusion.minimalFix;
        report.firstFailure.expectedValue = "finite values within physical HDR/LDR limits";
        report.firstFailure.actualValue = report.firstFailure.reason;

        for (const auto& pass : forensicReport.passes) {
            if (pass.passId != failPass) continue;
            if (!pass.metadata.pixelShader.empty()) report.firstFailure.shader = pass.metadata.pixelShader;
            else if (!pass.metadata.computeShader.empty()) report.firstFailure.shader = pass.metadata.computeShader;
            else if (!pass.metadata.vertexShader.empty()) report.firstFailure.shader = pass.metadata.vertexShader;
            if (!pass.callStack.empty()) report.firstFailure.function = pass.callStack.front();
            report.firstFailure.variable = "framebuffer RGB";
            break;
        }
    }

    for (const auto& stage : report.pixelPipeline) {
        if (!stage.firstInvalidStage) continue;
        if (!report.firstFailure.detected) {
            report.firstFailure.detected = true;
            report.firstFailure.pass = static_cast<int>(stage.passId);
            report.firstFailure.passName = stage.passName;
            report.firstFailure.reason = stage.invalidReason;
            report.firstFailure.variable = "probe pixel RGB";
            report.firstFailure.actualValue = FmtRgb(stage.hdrRgb.r, stage.hdrRgb.g, stage.hdrRgb.b);
            report.firstFailure.expectedValue = "finite non-negative radiance";
        }
        break;
    }

    for (const auto& pass : report.passValidations) {
        for (const auto& err : pass.abnormalities) {
            report.validationWarnings.push_back("[" + pass.passName + "] " + err);
        }
    }
}

void RenderGpuInvestigator::BuildPixelPipeline(
    GpuInvestigationReport& report,
    const ForensicFrameReport& forensicReport,
    const Renderer::CameraUniform& camera,
    const glm::vec3& cameraForward,
    const SceneEnvironmentUniform& env) const {

    report.pixelPipeline.clear();
    bool foundFirstInvalid = false;

    const ForensicPassId pipelineOrder[] = {
        ForensicPassId::Clear,
        ForensicPassId::AtmosphereLUT,
        ForensicPassId::SkyAtmosphere,
        ForensicPassId::Geometry,
        ForensicPassId::VolumetricClouds,
        ForensicPassId::Fog,
        ForensicPassId::Lighting,
        ForensicPassId::Exposure,
        ForensicPassId::ToneMapping,
        ForensicPassId::Bloom,
        ForensicPassId::UI,
        ForensicPassId::Present,
    };

    for (const ForensicPassId orderedPass : pipelineOrder) {
        const ForensicPassExecution* exec = nullptr;
        for (const auto& pass : forensicReport.passes) {
            if (pass.passId == orderedPass && pass.executed) {
                exec = &pass;
                break;
            }
        }
        if (!exec) continue;

        GpuPixelPipelineStage stage{};
        stage.passId = exec->passId;
        stage.passName = exec->passName;
        stage.hdrRgb = exec->outputStats.valid ? exec->outputStats.probePixel : exec->outputStats.corners.center;
        if (exec->outputStats.valid && stage.hdrRgb.r == 0.0f && stage.hdrRgb.g == 0.0f && stage.hdrRgb.b == 0.0f) {
            stage.hdrRgb = exec->outputStats.corners.center;
        }
        stage.valid = exec->outputStats.valid || exec->executed;
        const bool isLdr = RenderForensics::IsLdrPass(exec->passId);
        if (isLdr) {
            stage.ldrRgb = stage.hdrRgb;
            stage.finalLdrRGB = glm::vec3(stage.hdrRgb.r, stage.hdrRgb.g, stage.hdrRgb.b);
        } else {
            stage.finalHdrRGB = glm::vec3(stage.hdrRgb.r, stage.hdrRgb.g, stage.hdrRgb.b);
        }
        stage.depth = report.probeDepth;

        if (exec->passId == ForensicPassId::SkyAtmosphere && report.shaderTraceValid) {
            const auto& t = report.shaderTrace;
            stage.viewDirection = t.viewDirection;
            stage.sunDirection = t.sunDirection;
            stage.skyViewUV = t.skyViewUV;
            stage.transmittanceUV = t.transmittanceUV;
            stage.rayleighRGB = t.rayleighRGB;
            stage.mieRGB = t.mieRGB;
            stage.multiScatterRGB = t.multiScatterRGB;
            stage.sunRGB = t.sunDiskRGB;
            stage.finalHdrRGB = t.finalHdrRGB;
        }

        if (stage.valid && !foundFirstInvalid) {
            const float maxHdr = 100.0f;
            if (IsPixelInvalid(stage.hdrRgb, isLdr, maxHdr)) {
                stage.firstInvalidStage = true;
                stage.invalidReason = "Probe pixel became invalid after " + stage.passName;
                foundFirstInvalid = true;
            }
        }

        report.pixelPipeline.push_back(stage);
    }

    (void)camera;
    (void)cameraForward;
    (void)env;
}

void RenderGpuInvestigator::BuildRenderGraph(GpuInvestigationReport& report) const {
    report.graphNodes.clear();
    report.graphEdges.clear();

    for (const auto& pass : report.passValidations) {
        GpuRenderGraphNode node{};
        node.passId = pass.passId;
        node.passName = pass.passName;
        node.executed = pass.executed;
        node.cpuMs = pass.cpuMs;
        node.gpuMs = pass.gpuMs;
        node.inputs = pass.inputTextures;
        node.outputs = pass.outputTextures;
        if (!pass.outputTextures.empty()) node.producedTarget = pass.outputTextures.front();
        report.graphNodes.push_back(node);
    }

    for (size_t i = 1; i < report.graphNodes.size(); ++i) {
        GpuRenderGraphEdge edge{};
        edge.fromPass = report.graphNodes[i - 1].passName;
        edge.toPass = report.graphNodes[i].passName;
        edge.viaResource = report.graphNodes[i - 1].producedTarget;
        report.graphEdges.push_back(edge);
    }
}

void RenderGpuInvestigator::BuildLutInspections(
    GpuInvestigationReport& report,
    const std::vector<RenderDebuggerLUTStats>& lutStats,
    const std::vector<GpuCachedLUTData>& cachedLuts) const {

    report.lutInspections.clear();
    for (size_t i = 0; i < lutStats.size(); ++i) {
        GpuLutInspection lut{};
        lut.name = lutStats[i].name;
        lut.stats = lutStats[i];
        lut.invalidPixelCount = lutStats[i].nanCount + lutStats[i].infCount + lutStats[i].negativeCount;

        if (i < cachedLuts.size() && !cachedLuts[i].rgba.empty()) {
            lut.luminanceHistogram = BuildLuminanceHistogram(cachedLuts[i].rgba, cachedLuts[i].width, cachedLuts[i].height);
            lut.previewRgba = BuildDownscaledPreview(
                cachedLuts[i].rgba, cachedLuts[i].width, cachedLuts[i].height, 64,
                lut.previewWidth, lut.previewHeight);
        }
        report.lutInspections.push_back(std::move(lut));
    }
}

void RenderGpuInvestigator::CompareWithBaseline(GpuInvestigationReport& report) {
    report.frameComparisons.clear();
    report.hasBaselineCapture = m_HasBaseline;
    report.hasComparisonCapture = m_RequestComparison || m_HasBaseline;

    if (!m_HasBaseline) return;

    auto addCmp = [&](const std::string& label, const std::string& before, const std::string& after) {
        GpuFrameComparisonEntry e{};
        e.label = label;
        e.before = before;
        e.after = after;
        e.changed = before != after;
        report.frameComparisons.push_back(e);
    };

    addCmp("Frame", std::to_string(m_BaselineReport.frameNumber), std::to_string(report.frameNumber));
    addCmp("Probe UV",
        std::to_string(m_BaselineReport.probeUV.x) + "," + std::to_string(m_BaselineReport.probeUV.y),
        std::to_string(report.probeUV.x) + "," + std::to_string(report.probeUV.y));

    const size_t targetCount = (std::min)(m_BaselineReport.renderTargets.size(), report.renderTargets.size());
    for (size_t i = 0; i < targetCount; ++i) {
        const auto& a = m_BaselineReport.renderTargets[i];
        const auto& b = report.renderTargets[i];
        if (!a.stats.valid || !b.stats.valid) continue;
        const std::string before = FmtRgb(a.stats.avgR, a.stats.avgG, a.stats.avgB);
        const std::string after = FmtRgb(b.stats.avgR, b.stats.avgG, b.stats.avgB);
        addCmp(a.displayName + " avgRGB", before, after);
    }

    if (m_RequestBaseline) {
        m_BaselineReport = report;
        m_HasBaseline = true;
        m_RequestBaseline = false;
    } else if (m_RequestComparison) {
        m_RequestComparison = false;
    }
}

GpuInvestigationReport RenderGpuInvestigator::BuildReport(
    const ForensicFrameReport& forensicReport,
    const std::vector<GpuResourceCatalogEntry>& catalog,
    const std::vector<RenderDebuggerLUTStats>& lutStats,
    const std::vector<GpuCachedLUTData>& cachedLuts,
    const Renderer::CameraUniform& camera,
    const glm::vec3& cameraForward,
    const SceneEnvironmentUniform& env,
    float maxHdrComponent,
    float probeDepth) {

    GpuInvestigationReport report{};
    report.frameNumber = forensicReport.frameIndex;
    report.probeUV = m_ProbeUV;
    report.probePixelX = m_ProbePixelX;
    report.probePixelY = m_ProbePixelY;
    report.probeDepth = probeDepth;
    report.selectedPreviewTargetIndex = m_SelectedPreviewTarget;

    for (const auto& pass : forensicReport.passes) {
        GpuPassValidation v{};
        v.passId = pass.passId;
        v.passName = pass.passName;
        v.executed = pass.executed;
        v.cpuMs = pass.cpuMs;
        v.gpuMs = pass.gpuMs;
        v.inputTextures = PassInputTextures(pass.passId);
        v.outputTextures = PassOutputTextures(pass.passId);
        v.outputStats = pass.outputStats;
        v.probePixel = pass.outputStats.probePixel;
        if (v.probePixel.r == 0.0f && v.probePixel.g == 0.0f && v.probePixel.b == 0.0f && pass.outputStats.valid) {
            v.probePixel = pass.outputStats.corners.center;
        }
        v.vertexShader = pass.metadata.vertexShader;
        v.pixelShader = pass.metadata.pixelShader;
        v.computeShader = pass.metadata.computeShader;
        report.passValidations.push_back(std::move(v));
    }

    MergeCatalogIntoTargets(report, catalog, forensicReport);

    report.shaderTrace = AtmosphereRadianceTracer::Get().TraceSample(
        "probe",
        m_ProbeUV,
        camera.view,
        camera.proj,
        camera.pos,
        env);
    report.shaderTraceValid = report.shaderTrace.valid;

    BuildPixelPipeline(report, forensicReport, camera, cameraForward, env);
    ValidatePasses(report, maxHdrComponent);
    DetectFirstFailure(report, forensicReport);
    BuildLutInspections(report, lutStats, cachedLuts);
    BuildRenderGraph(report);
    CompareWithBaseline(report);

    m_LastReport = report;
    return report;
}

#endif
} // namespace we::runtime::renderer
