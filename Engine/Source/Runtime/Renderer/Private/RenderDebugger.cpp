#include "Renderer/RenderDebugger.hpp"
#include "Renderer/RenderGpuInvestigator.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {

bool MatrixIsFinite(const glm::mat4& m) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            if (!std::isfinite(m[c][r])) return false;
        }
    }
    return true;
}

bool MatrixNearIdentity(const glm::mat4& m, float epsilon = 1e-3f) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            const float expected = (c == r) ? 1.0f : 0.0f;
            if (std::fabs(m[c][r] - expected) > epsilon) return false;
        }
    }
    return true;
}

float MatrixMaxAbsError(const glm::mat4& a, const glm::mat4& b) {
    float maxErr = 0.0f;
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            maxErr = (std::max)(maxErr, std::fabs(a[c][r] - b[c][r]));
        }
    }
    return maxErr;
}

std::string Mat4ToString(const glm::mat4& m) {
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    for (int r = 0; r < 4; ++r) {
        if (r > 0) ss << " | ";
        ss << m[0][r] << "," << m[1][r] << "," << m[2][r] << "," << m[3][r];
    }
    return ss.str();
}

glm::vec3 UnprojectWorldPosition(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos) {
    const glm::mat4 invVP = glm::inverse(proj * view);
    const glm::vec4 clip(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 world = invVP * clip;
    if (std::fabs(world.w) > 1e-6f) {
        world /= world.w;
    }
    (void)cameraPos;
    return glm::vec3(world);
}

} // namespace

RenderDebugger& RenderDebugger::Get() {
    static RenderDebugger instance;
    return instance;
}

RenderDebugger::RenderDebugger() {
    ResetPassGatesToDefault();
}

void RenderDebugger::SetBinaryIsolationEnabled(bool enabled) {
    m_BinaryIsolationEnabled = enabled;
}

void RenderDebugger::SetPassEnabled(ForensicPassId pass, bool enabled) {
    const int index = static_cast<int>(pass);
    if (index >= 0 && index < static_cast<int>(ForensicPassId::Count)) {
        m_PassEnabled[static_cast<size_t>(index)] = enabled;
    }
}

bool RenderDebugger::IsPassEnabled(ForensicPassId pass) const {
    const int index = static_cast<int>(pass);
    if (index < 0 || index >= static_cast<int>(ForensicPassId::Count)) return true;
    return m_PassEnabled[static_cast<size_t>(index)];
}

void RenderDebugger::ResetPassGatesToDefault() {
    m_PassEnabled.fill(true);
    m_PassEnabled[static_cast<size_t>(ForensicPassId::Shadow)] = false;
    m_PassEnabled[static_cast<size_t>(ForensicPassId::Gizmos)] = false;
}

bool RenderDebugger::ShouldRunForensicPass(ForensicPassId pass) const {
    if (!m_BinaryIsolationEnabled) return true;
    if (pass == ForensicPassId::Clear || pass == ForensicPassId::Present) return true;
    return IsPassEnabled(pass);
}

bool RenderDebugger::ShouldRunRenderPass(RenderPassId pass) const {
    if (!m_BinaryIsolationEnabled) return true;
    if (pass == RenderPassId::Clear || pass == RenderPassId::Present) return true;

    switch (pass) {
        case RenderPassId::SkyAtmosphere:
            return IsPassEnabled(ForensicPassId::SkyAtmosphere)
                || IsPassEnabled(ForensicPassId::AtmosphereLUT);
        case RenderPassId::Geometry:
            return IsPassEnabled(ForensicPassId::Geometry);
        case RenderPassId::Lighting:
            return IsPassEnabled(ForensicPassId::Lighting);
        case RenderPassId::VolumetricClouds:
            return IsPassEnabled(ForensicPassId::VolumetricClouds);
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective:
            return IsPassEnabled(ForensicPassId::Fog) || IsPassEnabled(ForensicPassId::AerialPerspective);
        case RenderPassId::PostProcessing:
        case RenderPassId::Exposure:
            return IsPassEnabled(ForensicPassId::Exposure)
                || IsPassEnabled(ForensicPassId::LuminanceReduce)
                || IsPassEnabled(ForensicPassId::Bloom)
                || IsPassEnabled(ForensicPassId::ToneMapping)
                || IsPassEnabled(ForensicPassId::SceneComposite);
        case RenderPassId::ToneMapping:
            return IsPassEnabled(ForensicPassId::ToneMapping);
        default:
            return true;
    }
}

void RenderDebugger::RequestCapture() {
    m_CaptureRequested = true;
}

bool RenderDebugger::ConsumeCaptureRequest() {
    const bool requested = m_CaptureRequested;
    m_CaptureRequested = false;
    return requested;
}

RenderDebuggerMatrixInfo RenderDebugger::ValidateMatrices(
    const glm::mat4& view,
    const glm::mat4& projection) {

    RenderDebuggerMatrixInfo info{};
    info.view = view;
    info.projection = projection;

    if (!MatrixIsFinite(view)) {
        info.viewValid = false;
        info.validationErrors.push_back("View matrix contains non-finite values");
    }
    if (!MatrixIsFinite(projection)) {
        info.projectionValid = false;
        info.validationErrors.push_back("Projection matrix contains non-finite values");
    }

    const float viewDet = glm::determinant(view);
    const float projDet = glm::determinant(projection);
    if (std::fabs(viewDet) < 1e-8f) {
        info.viewValid = false;
        info.validationErrors.push_back("View matrix is singular (det≈0)");
    }
    if (std::fabs(projDet) < 1e-8f) {
        info.projectionValid = false;
        info.validationErrors.push_back("Projection matrix is singular (det≈0)");
    }

    info.inverseView = glm::inverse(view);
    info.inverseProjection = glm::inverse(projection);

    if (!MatrixIsFinite(info.inverseView)) {
        info.inverseViewValid = false;
        info.validationErrors.push_back("Inverse view matrix inversion failed");
    } else {
        const glm::mat4 recomposed = view * info.inverseView;
        if (!MatrixNearIdentity(recomposed, 2e-2f)) {
            info.inverseViewValid = false;
            info.validationErrors.push_back("View * InverseView deviates from identity (max err="
                + std::to_string(MatrixMaxAbsError(recomposed, glm::mat4(1.0f))) + ")");
        }
    }

    if (!MatrixIsFinite(info.inverseProjection)) {
        info.inverseProjectionValid = false;
        info.validationErrors.push_back("Inverse projection matrix inversion failed");
    } else {
        const glm::mat4 recomposed = projection * info.inverseProjection;
        if (!MatrixNearIdentity(recomposed, 2e-2f)) {
            info.inverseProjectionValid = false;
            info.validationErrors.push_back("Projection * InverseProjection deviates from identity");
        }
    }

    return info;
}

RenderDebuggerLUTStats RenderDebugger::AnalyzeLUTRGBA(
    const char* name,
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height) {

    RenderDebuggerLUTStats stats{};
    stats.name = name ? name : "LUT";
    stats.width = width;
    stats.height = height;
    if (rgba.empty() || width == 0 || height == 0) return stats;

    stats.minR = stats.minG = stats.minB = 1e30f;
    stats.maxR = stats.maxG = stats.maxB = -1e30f;
    double sumR = 0.0, sumG = 0.0, sumB = 0.0;
    uint64_t count = 0;

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
            if (idx + 2 >= rgba.size()) continue;
            const float r = rgba[idx];
            const float g = rgba[idx + 1];
            const float b = rgba[idx + 2];
            ++count;
            if (std::isnan(r) || std::isnan(g) || std::isnan(b)) ++stats.nanCount;
            if (std::isinf(r) || std::isinf(g) || std::isinf(b)) ++stats.infCount;
            if (r < 0.0f || g < 0.0f || b < 0.0f) ++stats.negativeCount;
            stats.minR = (std::min)(stats.minR, r);
            stats.minG = (std::min)(stats.minG, g);
            stats.minB = (std::min)(stats.minB, b);
            stats.maxR = (std::max)(stats.maxR, r);
            stats.maxG = (std::max)(stats.maxG, g);
            stats.maxB = (std::max)(stats.maxB, b);
            sumR += r;
            sumG += g;
            sumB += b;
        }
    }

    if (count > 0) {
        stats.avgR = static_cast<float>(sumR / static_cast<double>(count));
        stats.avgG = static_cast<float>(sumG / static_cast<double>(count));
        stats.avgB = static_cast<float>(sumB / static_cast<double>(count));
        stats.valid = true;
    }
    return stats;
}

void RenderDebugger::RecordCapture(const RenderDebuggerFrameSnapshot& snapshot) {
    if (!m_HasPreviousCapture) {
        m_PreviousCapture = snapshot;
        m_HasPreviousCapture = true;
        return;
    }

    m_LastSnapshot.captureDiffs.clear();
    const auto diffResources = [&](const char* label,
                                   const RenderDebuggerResourceInfo& prev,
                                   const RenderDebuggerResourceInfo& curr) {
        if (!prev.valid || !curr.valid) return;
        RenderDebuggerCaptureDiff diff{};
        diff.resourceName = label;
        diff.maxDeltaR = std::fabs(curr.avgR - prev.avgR);
        diff.maxDeltaG = std::fabs(curr.avgG - prev.avgG);
        diff.maxDeltaB = std::fabs(curr.avgB - prev.avgB);
        diff.changed = diff.maxDeltaR > 1e-4f || diff.maxDeltaG > 1e-4f || diff.maxDeltaB > 1e-4f;
        m_LastSnapshot.captureDiffs.push_back(diff);
    };

    if (!m_PreviousCapture.resources.empty() && !snapshot.resources.empty()) {
        const size_t count = (std::min)(m_PreviousCapture.resources.size(), snapshot.resources.size());
        for (size_t i = 0; i < count; ++i) {
            diffResources(snapshot.resources[i].name.c_str(),
                m_PreviousCapture.resources[i], snapshot.resources[i]);
        }
    }

    for (size_t i = 0; i < snapshot.luts.size() && i < m_PreviousCapture.luts.size(); ++i) {
        if (!snapshot.luts[i].valid || !m_PreviousCapture.luts[i].valid) continue;
        RenderDebuggerCaptureDiff diff{};
        diff.resourceName = snapshot.luts[i].name;
        diff.maxDeltaR = std::fabs(snapshot.luts[i].avgR - m_PreviousCapture.luts[i].avgR);
        diff.maxDeltaG = std::fabs(snapshot.luts[i].avgG - m_PreviousCapture.luts[i].avgG);
        diff.maxDeltaB = std::fabs(snapshot.luts[i].avgB - m_PreviousCapture.luts[i].avgB);
        diff.changed = diff.maxDeltaR > 1e-5f || diff.maxDeltaG > 1e-5f || diff.maxDeltaB > 1e-5f;
        m_LastSnapshot.captureDiffs.push_back(diff);
    }

    m_PreviousCapture = snapshot;
}

RenderDebuggerFrameSnapshot RenderDebugger::BuildSnapshot(
    const ForensicFrameReport& forensicReport,
    const FrameStats& frameStats,
    const Renderer::CameraUniform& camera,
    const glm::vec3& cameraForward,
    const glm::vec3& cameraRight,
    const glm::vec3& cameraUp,
    float nearPlane,
    float farPlane,
    float fovDegrees,
    const SceneEnvironmentUniform& env,
    const std::vector<RenderDebuggerLUTStats>& lutStats,
    const std::vector<GpuCachedLUTData>& cachedLuts,
    const std::vector<GpuResourceCatalogEntry>& resourceCatalog,
    float fps,
    uint32_t offscreenWidth,
    uint32_t offscreenHeight,
    float probeDepth,
    float maxHdrComponent) {

    RenderDebuggerFrameSnapshot snapshot{};
    snapshot.frameNumber = forensicReport.frameIndex;
    snapshot.fps = fps;
    snapshot.frameTimeMs = frameStats.cpuFrameMs;
    snapshot.cpuTimeMs = frameStats.cpuFrameMs;
    snapshot.gpuTimeMs = frameStats.gpuFrameMs;

    snapshot.camera.position = camera.pos;
    snapshot.camera.forward = cameraForward;
    snapshot.camera.right = cameraRight;
    snapshot.camera.up = cameraUp;
    snapshot.camera.nearPlane = nearPlane;
    snapshot.camera.farPlane = farPlane;
    snapshot.camera.fovDegrees = fovDegrees;
    snapshot.camera.matrices = ValidateMatrices(camera.view, camera.proj);

    snapshot.environment.sunDirection = env.sunDirection;
    snapshot.environment.sunIntensity = env.sunIntensity;
    snapshot.environment.sunAngularRadius = env.sunAngularRadius;
    snapshot.environment.skyIntensity = env.skyLightIntensity;
    snapshot.environment.planetRadius = env.planetRadius;
    snapshot.environment.atmosphereRadius = env.planetRadius + env.atmosphereHeight;
    snapshot.environment.rayleighScaleHeight = 8.0f;
    snapshot.environment.mieScaleHeight = 1.2f;
    snapshot.environment.ozoneDensity = env.ozoneAbsorption.y;

    snapshot.exposure.autoExposure = env.enableAutoExposure > 0.5f;
    snapshot.exposure.manualExposureEV = frameStats.manualExposureEV;
    snapshot.exposure.ev100 = forensicReport.cameraLog.ev100;
    snapshot.exposure.exposureMultiplier = forensicReport.cameraLog.exposureMultiplier;
    snapshot.exposure.averageLuminance = forensicReport.cameraLog.avgSceneLuminance;

    if (forensicReport.cameraLog.hdrBeforeExposure.valid) {
        const auto& hdr = forensicReport.cameraLog.hdrBeforeExposure;
        snapshot.exposure.histogramMin = hdr.minR;
        snapshot.exposure.histogramMax = hdr.maxR;
        snapshot.exposure.histogramAverage = hdr.avgLuminance;
    } else {
        snapshot.exposure.histogramAverage = frameStats.gpuAverageLuminance;
    }

    snapshot.atmosphere = frameStats.atmosphereProbe;
    snapshot.luts = lutStats;

    snapshot.centerPixel.viewDirection = frameStats.atmosphereProbe.viewDirection;
    snapshot.centerPixel.worldPosition = UnprojectWorldPosition(camera.view, camera.proj, camera.pos);
    snapshot.centerPixel.skyViewUV = frameStats.atmosphereProbe.skyViewUV;
    snapshot.centerPixel.transmittanceUV = frameStats.atmosphereProbe.transmittanceUV;
    snapshot.centerPixel.rayleighRGB = frameStats.atmosphereProbe.rayleighRGB;
    snapshot.centerPixel.mieRGB = frameStats.atmosphereProbe.mieRGB;
    snapshot.centerPixel.multiScatterRGB = frameStats.atmosphereProbe.multiScatterRGB;
    snapshot.centerPixel.sunDiskRGB = frameStats.atmosphereProbe.sunRGB;
    snapshot.centerPixel.finalHdrRGB = frameStats.atmosphereProbe.finalHdrRGB;
    snapshot.centerPixel.valid = frameStats.atmosphereProbe.valid;

    for (const auto& pass : forensicReport.passes) {
        RenderDebuggerPassNode node{};
        node.passId = pass.passId;
        node.passName = pass.passName;
        node.executed = pass.executed;
        node.cpuMs = pass.cpuMs;
        node.gpuMs = pass.gpuMs;
        node.renderTarget = pass.metadata.outputTarget;
        node.textureFormat = pass.metadata.format;
        node.inputCount = pass.inputStats.valid ? 1 : 0;
        node.outputCount = pass.outputStats.valid ? 1 : (pass.executed ? 1 : 0);
        node.health = pass.health;
        node.validationErrors = pass.validationErrors;
        snapshot.renderGraph.push_back(std::move(node));

        if (pass.outputStats.valid) {
            RenderDebuggerResourceInfo resource{};
            resource.name = pass.passName + " / " + pass.metadata.outputTarget;
            resource.width = pass.metadata.width ? pass.metadata.width : offscreenWidth;
            resource.height = pass.metadata.height ? pass.metadata.height : offscreenHeight;
            resource.format = pass.metadata.format;
            resource.memoryBytes = static_cast<uint64_t>(resource.width) * resource.height * 8;
            resource.minR = pass.outputStats.minR;
            resource.minG = pass.outputStats.minG;
            resource.minB = pass.outputStats.minB;
            resource.maxR = pass.outputStats.maxR;
            resource.maxG = pass.outputStats.maxG;
            resource.maxB = pass.outputStats.maxB;
            resource.avgR = pass.outputStats.avgR;
            resource.avgG = pass.outputStats.avgG;
            resource.avgB = pass.outputStats.avgB;
            resource.valid = true;
            snapshot.resources.push_back(std::move(resource));
        }
    }

    for (const auto& err : snapshot.camera.matrices.validationErrors) {
        snapshot.validationWarnings.push_back("[Camera] " + err);
    }

    const float sunLen = glm::length(glm::vec3(env.sunDirection));
    if (std::fabs(sunLen - 1.0f) > 0.01f) {
        snapshot.validationWarnings.push_back("[Environment] Sun direction length != 1 (len=" + std::to_string(sunLen) + ")");
    }
    if (frameStats.atmosphereProbe.valid) {
        if (std::fabs(frameStats.atmosphereProbe.viewDirectionLength - 1.0f) > 0.01f) {
            snapshot.validationWarnings.push_back("[Atmosphere] View direction length != 1");
        }
        if (frameStats.atmosphereProbe.skyViewUV.x < 0.0f || frameStats.atmosphereProbe.skyViewUV.x > 1.0f
            || frameStats.atmosphereProbe.skyViewUV.y < 0.0f || frameStats.atmosphereProbe.skyViewUV.y > 1.0f) {
            snapshot.validationWarnings.push_back("[Atmosphere] SkyView UV outside [0,1]");
        }
        if (frameStats.atmosphereProbe.transmittanceUV.x < 0.0f || frameStats.atmosphereProbe.transmittanceUV.x > 1.0f
            || frameStats.atmosphereProbe.transmittanceUV.y < 0.0f || frameStats.atmosphereProbe.transmittanceUV.y > 1.0f) {
            snapshot.validationWarnings.push_back("[Atmosphere] Transmittance UV outside [0,1]");
        }
    }

    for (const auto& lut : lutStats) {
        if (!lut.valid) continue;
        if (lut.nanCount > 0) snapshot.validationWarnings.push_back("[LUT " + lut.name + "] NaN count=" + std::to_string(lut.nanCount));
        if (lut.infCount > 0) snapshot.validationWarnings.push_back("[LUT " + lut.name + "] Inf count=" + std::to_string(lut.infCount));
        if (lut.negativeCount > 0) snapshot.validationWarnings.push_back("[LUT " + lut.name + "] Negative count=" + std::to_string(lut.negativeCount));
    }

    for (const auto& pass : forensicReport.passes) {
        for (const auto& err : pass.validationErrors) {
            snapshot.validationWarnings.push_back("[" + pass.passName + "] " + err);
        }
        if (pass.outputStats.valid) {
            if (pass.outputStats.nanCount > 0) {
                snapshot.validationWarnings.push_back("[" + pass.passName + "] NaN in render target");
            }
            if (pass.outputStats.infCount > 0) {
                snapshot.validationWarnings.push_back("[" + pass.passName + "] Inf in render target");
            }
        }
    }

    snapshot.firstFailure.detected = forensicReport.frameFailed
        || forensicReport.conclusion.firstInvalidOutputPass != ForensicPassId::Count;
    if (snapshot.firstFailure.detected) {
        const ForensicPassId failPass = forensicReport.firstAnomalyPass != ForensicPassId::Count
            ? forensicReport.firstAnomalyPass
            : forensicReport.conclusion.firstInvalidOutputPass;
        snapshot.firstFailure.pass = static_cast<int>(failPass);
        snapshot.firstFailure.passName = RenderForensics::PassName(failPass);
        snapshot.firstFailure.reason = !forensicReport.firstAnomalyReason.empty()
            ? forensicReport.firstAnomalyReason
            : forensicReport.conclusion.firstInvalidOutputReason;
        snapshot.firstFailure.sourceFile = !forensicReport.rootCauseFile.empty()
            ? forensicReport.rootCauseFile
            : forensicReport.conclusion.rootCauseFile;
        snapshot.firstFailure.function = forensicReport.conclusion.rootCauseFunction;
        snapshot.firstFailure.minimalFix = !forensicReport.minimalFix.empty()
            ? forensicReport.minimalFix
            : forensicReport.conclusion.minimalFix;

        for (const auto& pass : forensicReport.passes) {
            if (pass.passId != failPass) continue;
            if (!pass.metadata.pixelShader.empty()) snapshot.firstFailure.shader = pass.metadata.pixelShader;
            else if (!pass.metadata.computeShader.empty()) snapshot.firstFailure.shader = pass.metadata.computeShader;
            else if (!pass.metadata.vertexShader.empty()) snapshot.firstFailure.shader = pass.metadata.vertexShader;
            if (!pass.metadata.sourceFile.empty()) snapshot.firstFailure.sourceFile = pass.metadata.sourceFile;
            if (!pass.callStack.empty()) snapshot.firstFailure.function = pass.callStack.front();
            if (!pass.validationErrors.empty()) {
                snapshot.firstFailure.variable = "framebuffer RGB";
                snapshot.firstFailure.actualValue = pass.validationErrors.front();
                snapshot.firstFailure.expectedValue = "finite HDR/LDR within physical limits";
            }
            break;
        }
    }

    snapshot.binaryIsolationActive = m_BinaryIsolationEnabled;
    snapshot.passEnabled = m_PassEnabled;

    snapshot.gpuInvestigation = RenderGpuInvestigator::Get().BuildReport(
        forensicReport,
        resourceCatalog,
        lutStats,
        cachedLuts,
        camera,
        cameraForward,
        env,
        maxHdrComponent,
        probeDepth);

    for (auto& rt : snapshot.gpuInvestigation.renderTargets) {
        for (const auto& lut : snapshot.gpuInvestigation.lutInspections) {
            if (rt.displayName.find(lut.name) != std::string::npos
                || rt.id.find(lut.stats.name) != std::string::npos) {
                rt.previewRgba = lut.previewRgba;
                rt.previewWidth = lut.previewWidth;
                rt.previewHeight = lut.previewHeight;
                if (lut.stats.valid) {
                    rt.stats.valid = true;
                    rt.stats.minR = lut.stats.minR;
                    rt.stats.minG = lut.stats.minG;
                    rt.stats.minB = lut.stats.minB;
                    rt.stats.maxR = lut.stats.maxR;
                    rt.stats.maxG = lut.stats.maxG;
                    rt.stats.maxB = lut.stats.maxB;
                    rt.stats.avgR = lut.stats.avgR;
                    rt.stats.avgG = lut.stats.avgG;
                    rt.stats.avgB = lut.stats.avgB;
                    rt.stats.nanCount = lut.stats.nanCount;
                    rt.stats.infCount = lut.stats.infCount;
                    rt.stats.negativeCount = lut.stats.negativeCount;
                }
            }
        }
    }

    if (snapshot.gpuInvestigation.firstFailure.detected) {
        snapshot.firstFailure = snapshot.gpuInvestigation.firstFailure;
    }
    for (const auto& warn : snapshot.gpuInvestigation.validationWarnings) {
        snapshot.validationWarnings.push_back(warn);
    }

    if (m_CaptureRequested || m_HasPreviousCapture) {
        RecordCapture(snapshot);
        if (m_CaptureRequested) {
            snapshot.captureCount = static_cast<int>(m_LastSnapshot.captureDiffs.size());
            snapshot.captureStatus = "Capture recorded — compare with previous frame below";
            m_CaptureRequested = false;
        }
    }
    snapshot.captureDiffs = m_LastSnapshot.captureDiffs;

    m_LastSnapshot = snapshot;
    return snapshot;
}

#endif
} // namespace we::runtime::renderer
