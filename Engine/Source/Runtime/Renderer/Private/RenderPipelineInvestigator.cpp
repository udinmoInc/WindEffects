#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/RendererConfig.hpp"
#include "Core/LogCategory.hpp"
#include "Core/DiagnosticMacros.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

namespace {
VkDevice s_CachedDevice = VK_NULL_HANDLE;

float HalfToFloat(std::uint16_t value) {
    const std::uint32_t sign = (value >> 15) & 0x1;
    const std::uint32_t exponent = (value >> 10) & 0x1F;
    const std::uint32_t mantissa = value & 0x3FF;
    if (exponent == 0) {
        if (mantissa == 0) return sign ? -0.0f : 0.0f;
        const float result = std::ldexp(static_cast<float>(mantissa), -24);
        return sign ? -result : result;
    }
    if (exponent == 31) return mantissa == 0 ? (sign ? -INFINITY : INFINITY) : NAN;
    const std::uint32_t floatBits = (sign << 31) | ((exponent + (127 - 15)) << 23) | (mantissa << 13);
    float result = 0.0f;
    std::memcpy(&result, &floatBits, sizeof(result));
    return result;
}

bool StartsWith(const char* value, const char* prefix) {
    return value != nullptr && prefix != nullptr && std::strncmp(value, prefix, std::strlen(prefix)) == 0;
}

const char* EffectStepName(int step) {
    switch (step) {
        case 0: return "ClearOnly";
        case 1: return "Clear+Sky";
        case 2: return "Clear+Sky+Geometry";
        case 3: return "Clear+Sky+Geometry+Clouds";
        case 4: return "Clear+Sky+Geometry+Clouds+Fog";
        case 5: return "Clear+Sky+Geometry+Clouds+Fog+Post";
        default: return "Custom";
    }
}

bool EffectStepAllowsPass(int step, RenderPassId pass) {
    switch (pass) {
        case RenderPassId::Clear:
            return true;
        case RenderPassId::SkyAtmosphere:
            return step >= 1;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting:
            return step >= 2;
        case RenderPassId::VolumetricClouds:
            return step >= 3;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective:
            return step >= 4;
        case RenderPassId::PostProcessing:
        case RenderPassId::Exposure:
        case RenderPassId::ToneMapping:
            return step >= 5;
        default:
            return true;
    }
}

} // namespace

RenderPipelineInvestigator& RenderPipelineInvestigator::Get() {
    static RenderPipelineInvestigator instance;
    return instance;
}

const char* RenderPipelineInvestigator::PassName(RenderPassId pass) {
    switch (pass) {
        case RenderPassId::Clear: return "Clear";
        case RenderPassId::SkyAtmosphere: return "SkyAtmosphere";
        case RenderPassId::Geometry: return "Geometry";
        case RenderPassId::Lighting: return "Lighting";
        case RenderPassId::VolumetricClouds: return "VolumetricClouds";
        case RenderPassId::Fog: return "Fog";
        case RenderPassId::AerialPerspective: return "AerialPerspective";
        case RenderPassId::PostProcessing: return "PostProcessing";
        case RenderPassId::Exposure: return "Exposure";
        case RenderPassId::ToneMapping: return "ToneMapping";
        case RenderPassId::Present: return "Present";
        default: return "Unknown";
    }
}

RenderPipelineInvestigatorSettings RenderPipelineInvestigator::ParseCommandLine(int argc, char* argv[]) {
    RenderPipelineInvestigatorSettings settings{};
    if (const char* env = std::getenv("WE_PIPELINE_INVESTIGATION")) {
        if (env[0] == '1' || std::strcmp(env, "true") == 0) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
        }
    }
    if (const char* envStep = std::getenv("WE_PIPELINE_EFFECT_STEP")) {
        settings.enabled = true;
        settings.effectStep = std::atoi(envStep);
        settings.haltOnInvalid = false;
    }
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-PipelineInvestigation") == 0 || std::strcmp(argv[i], "--pipeline-investigation") == 0) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineStripAll") == 0 || std::strcmp(argv[i], "--pipeline-strip-all") == 0) {
            settings.enabled = true;
            settings.effectStep = 0;
            settings.haltOnInvalid = false;
            settings.logEveryFrame = false;
            settings.writeReportFile = false;
            continue;
        }
        if (StartsWith(argv[i], "-PipelineEffectStep=")) {
            settings.enabled = true;
            settings.effectStep = std::atoi(argv[i] + std::strlen("-PipelineEffectStep="));
            settings.haltOnInvalid = false;
            settings.logEveryFrame = false;
            settings.writeReportFile = false;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoHalt") == 0) {
            settings.haltOnInvalid = false;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoSky") == 0) {
            settings.enabled = true;
            settings.disableSky = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoPost") == 0) {
            settings.enabled = true;
            settings.disablePostProcessing = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoGrid") == 0) {
            settings.enabled = true;
            settings.disableGrid = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoClouds") == 0) {
            settings.enabled = true;
            settings.disableClouds = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoFog") == 0) {
            settings.enabled = true;
            settings.disableFog = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineNoGeometry") == 0) {
            settings.enabled = true;
            settings.disableGeometry = true;
            continue;
        }
        if (StartsWith(argv[i], "-PipelineOnlyPass=")) {
            settings.enabled = true;
            settings.enableGpuReadback = true;
            settings.onlyPass = std::atoi(argv[i] + std::strlen("-PipelineOnlyPass="));
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineConstantBlueSky") == 0) {
            settings.enabled = true;
            settings.forceConstantBlueSky = true;
            settings.disableGeometry = true;
            settings.disableClouds = true;
            settings.disableFog = true;
            settings.disableBloom = true;
            settings.forceExposureOne = true;
            settings.disableAutoExposure = true;
            settings.bypassToneMapping = true;
            continue;
        }
        if (std::strcmp(argv[i], "-PipelineForceExposureOne") == 0) {
            settings.enabled = true;
            settings.forceExposureOne = true;
            settings.disableAutoExposure = true;
        }
        if (std::strcmp(argv[i], "-PipelineBypassToneMap") == 0) {
            settings.enabled = true;
            settings.bypassToneMapping = true;
        }
    }
    return settings;
}

void RenderPipelineInvestigator::Configure(const RenderPipelineInvestigatorSettings& settings) {
    m_Settings = settings;
    m_WarmupRemaining = settings.warmupFrames;
    m_ShouldHalt = false;
    if (!settings.enabled) return;
    if (settings.effectStep >= 0) {
        WE_LOG_INFO(LogCategory::Renderer.data(),
            std::string("Render pipeline effect step ") + std::to_string(settings.effectStep)
                + " (" + EffectStepName(settings.effectStep) + "). Add effects one at a time with -PipelineEffectStep=N.");
    } else {
        WE_LOG_INFO(LogCategory::Renderer.data(), "Render pipeline debug overrides enabled.");
    }
    if (settings.enableGpuReadback) {
        WE_LOG_INFO(LogCategory::Renderer.data(), "Render pipeline GPU readback investigation enabled.");
    }
}

void RenderPipelineInvestigator::BeginFrame(uint64_t frameIndex) {
    if (s_CachedDevice != VK_NULL_HANDLE) {
        for (auto& cp : m_Pending) {
            if (cp.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(s_CachedDevice, cp.stagingBuffer, nullptr);
            if (cp.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(s_CachedDevice, cp.stagingMemory, nullptr);
        }
    }
    m_Pending.clear();
    m_Audit = {};
    m_LastReport = {};
    m_LastReport.frameIndex = frameIndex;
    if (m_WarmupRemaining > 0) --m_WarmupRemaining;
}

void RenderPipelineInvestigator::DestroyPending(const VulkanContext& context) {
    const VkDevice device = context.GetDevice();
    s_CachedDevice = device;
    for (auto& cp : m_Pending) {
        if (cp.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, cp.stagingBuffer, nullptr);
        if (cp.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(device, cp.stagingMemory, nullptr);
        cp.stagingBuffer = VK_NULL_HANDLE;
        cp.stagingMemory = VK_NULL_HANDLE;
    }
}

void RenderPipelineInvestigator::ApplyEnvironmentOverrides(SceneEnvironmentUniform& uniform) const {
    if (!m_Settings.enabled) return;
    if (m_Settings.forceConstantBlueSky) uniform.atmosphereDebugMode = 104;
    if (m_Settings.disableClouds) uniform.enableClouds = 0.0f;
    if (m_Settings.disableFog) uniform.enableVolumetricFog = 0.0f;
    if (m_Settings.disableBloom) uniform.bloomIntensity = 0.0f;
    if (m_Settings.disableAutoExposure || m_Settings.forceExposureOne) uniform.enableAutoExposure = 0.0f;
    if (m_Settings.forceExposureOne) {
        uniform.exposureEV = 0.0f;
        uniform.exposureCompensation = 0.0f;
    }
    if (m_Settings.bypassToneMapping) uniform.pipelineBypassToneMapping = 1;
    else uniform.pipelineBypassToneMapping = 0;
}

bool RenderPipelineInvestigator::ShouldRunPass(RenderPassId pass) const {
    if (!m_Settings.enabled) return true;
    if (m_Settings.onlyPass >= 0) {
        const int only = m_Settings.onlyPass;
        if (pass == RenderPassId::Clear) return true;
        return static_cast<int>(pass) == only;
    }
    if (m_Settings.effectStep >= 0) {
        return EffectStepAllowsPass(m_Settings.effectStep, pass);
    }
    switch (pass) {
        case RenderPassId::SkyAtmosphere:
            return !m_Settings.disableSky;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting:
            return !m_Settings.disableGeometry;
        case RenderPassId::VolumetricClouds:
            return !m_Settings.disableClouds;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective:
            return !m_Settings.disableFog && !m_Settings.disableAerialPerspective;
        case RenderPassId::PostProcessing:
        case RenderPassId::Exposure:
        case RenderPassId::ToneMapping:
            return !m_Settings.disablePostProcessing;
        default:
            return true;
    }
}

bool RenderPipelineInvestigator::ShouldRenderGrid() const {
    if (!m_Settings.enabled) return true;
    if (m_Settings.effectStep >= 0) return false;
    return !m_Settings.disableGrid;
}

void RenderPipelineInvestigator::RecordCameraAndEnvironment(const glm::vec3& cameraPos, const SceneEnvironmentUniform& env, float gpuAvgLuminance) {
    m_CameraLog.cameraX = cameraPos.x;
    m_CameraLog.cameraY = cameraPos.y;
    m_CameraLog.cameraZ = cameraPos.z;
    m_CameraLog.cameraHeight = cameraPos.y - env.worldOrigin.y;
    m_CameraLog.planetRadius = env.planetRadius;
    m_CameraLog.atmosphereRadius = env.planetRadius + env.atmosphereHeight;
    m_CameraLog.sunDirX = env.sunDirection.x;
    m_CameraLog.sunDirY = env.sunDirection.y;
    m_CameraLog.sunDirZ = env.sunDirection.z;
    m_CameraLog.sunIntensity = env.sunIntensity;
    m_CameraLog.exposureEV = env.exposureEV;
    m_CameraLog.hdrSkyLuminance = env.hdrSkyLuminance;
    m_CameraLog.avgLuminanceGpu = gpuAvgLuminance;
}

void RenderPipelineInvestigator::AuditPassBegin(RenderPassId pass) {
    if (!m_Settings.enabled) return;
    switch (pass) {
        case RenderPassId::Clear: ++m_Audit.clearPassCount; break;
        case RenderPassId::SkyAtmosphere: ++m_Audit.skyPassCount; break;
        case RenderPassId::Geometry:
        case RenderPassId::Lighting: ++m_Audit.geometryPassCount; break;
        case RenderPassId::VolumetricClouds: ++m_Audit.cloudsPassCount; break;
        case RenderPassId::Fog:
        case RenderPassId::AerialPerspective: ++m_Audit.fogPassCount; break;
        case RenderPassId::Exposure: ++m_Audit.exposurePassCount; break;
        case RenderPassId::ToneMapping: ++m_Audit.toneMapPassCount; break;
        case RenderPassId::Present: ++m_Audit.presentPassCount; break;
        default: break;
    }
}

void RenderPipelineInvestigator::AuditPassEnd(RenderPassId) {}

void RenderPipelineInvestigator::EnqueueCheckpoint(
    VkCommandBuffer cmd,
    const VulkanContext& context,
    VkImage colorImage,
    uint32_t width,
    uint32_t height,
    RenderPassId pass) {

    if (!m_Settings.enabled || m_WarmupRemaining > 0 || colorImage == VK_NULL_HANDLE || width == 0 || height == 0
        || !m_Settings.enableGpuReadback) return;

    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * height;

    PendingCheckpoint cp{};
    cp.pass = pass;
    cp.width = width;
    cp.height = height;
    context.CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        cp.stagingBuffer, cp.stagingMemory);

    const VkImageLayout srcLayout = (pass == RenderPassId::Clear || pass == RenderPassId::SkyAtmosphere || pass == RenderPassId::Geometry || pass == RenderPassId::VolumetricClouds || pass == RenderPassId::Fog)
        ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    context.CmdTransitionImageLayout(cmd, colorImage, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };
    vkCmdCopyImageToBuffer(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cp.stagingBuffer, 1, &region);

    context.CmdTransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout);

    m_Pending.push_back(cp);
}

HdrBufferStats RenderPipelineInvestigator::AnalyzePixels(const std::vector<float>& rgba, float maxComponent) const {
    HdrBufferStats stats{};
    if (rgba.empty()) return stats;
    const int stride = std::max(1, m_Settings.sampleStride);
    stats.minR = stats.minG = stats.minB = 1e30f;
    stats.maxR = stats.maxG = stats.maxB = -1e30f;
    double lumSum = 0.0;
    for (size_t i = 0; i + 2 < rgba.size(); i += static_cast<size_t>(stride) * 4) {
        const float r = rgba[i];
        const float g = rgba[i + 1];
        const float b = rgba[i + 2];
        ++stats.samples;
        if (std::isnan(r) || std::isnan(g) || std::isnan(b)) ++stats.nanCount;
        if (std::isinf(r) || std::isinf(g) || std::isinf(b)) ++stats.infCount;
        if (r < 0.0f || g < 0.0f || b < 0.0f) ++stats.negativeCount;
        if (r > maxComponent || g > maxComponent || b > maxComponent) ++stats.overLimitCount;
        stats.minR = std::min(stats.minR, r); stats.minG = std::min(stats.minG, g); stats.minB = std::min(stats.minB, b);
        stats.maxR = std::max(stats.maxR, r); stats.maxG = std::max(stats.maxG, g); stats.maxB = std::max(stats.maxB, b);
        lumSum += 0.2126 * r + 0.7152 * g + 0.0722 * b;
    }
    stats.avgLuminance = stats.samples > 0 ? static_cast<float>(lumSum / stats.samples) : 0.0f;
    stats.valid = stats.samples > 0;
    return stats;
}

bool RenderPipelineInvestigator::ValidateStats(const HdrBufferStats& stats, bool isLdr, std::string& outReason) const {
    if (!stats.valid) { outReason = "no samples"; return false; }
    if (stats.nanCount > 0) { outReason = "NaN in RGB (" + std::to_string(stats.nanCount) + " samples)"; return false; }
    if (stats.infCount > 0) { outReason = "Inf in RGB (" + std::to_string(stats.infCount) + " samples)"; return false; }
    if (stats.negativeCount > 0) { outReason = "negative RGB (" + std::to_string(stats.negativeCount) + " samples)"; return false; }
    const float limit = isLdr ? 1.05f : m_Settings.maxHdrComponent;
    if (stats.overLimitCount > 0) {
        outReason = "component exceeds limit " + std::to_string(limit)
            + " max=(" + std::to_string(stats.maxR) + "," + std::to_string(stats.maxG) + "," + std::to_string(stats.maxB) + ")";
        return false;
    }
    return true;
}

HdrBufferStats RenderPipelineInvestigator::ReadbackStaging(const VulkanContext& context, const PendingCheckpoint& cp) const {
    HdrBufferStats stats{};
    if (cp.stagingBuffer == VK_NULL_HANDLE) return stats;
    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((cp.width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * cp.height;
    void* mapped = nullptr;
    vkMapMemory(context.GetDevice(), cp.stagingMemory, 0, bufferSize, 0, &mapped);
    std::vector<float> rgba(cp.width * cp.height * 4);
    for (uint32_t y = 0; y < cp.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(mapped) + y * rowPitch;
        for (uint32_t x = 0; x < cp.width; ++x) {
            const auto* pixel = row + x * bytesPerPixel;
            const auto r16 = static_cast<std::uint16_t>(pixel[0] | (pixel[1] << 8));
            const auto g16 = static_cast<std::uint16_t>(pixel[2] | (pixel[3] << 8));
            const auto b16 = static_cast<std::uint16_t>(pixel[4] | (pixel[5] << 8));
            const size_t idx = (static_cast<size_t>(y) * cp.width + x) * 4;
            rgba[idx] = HalfToFloat(r16);
            rgba[idx + 1] = HalfToFloat(g16);
            rgba[idx + 2] = HalfToFloat(b16);
            rgba[idx + 3] = 1.0f;
        }
    }
    vkUnmapMemory(context.GetDevice(), cp.stagingMemory);
    const bool isLdr = (cp.pass == RenderPassId::ToneMapping || cp.pass == RenderPassId::Present);
    return AnalyzePixels(rgba, isLdr ? 1.05f : m_Settings.maxHdrComponent);
}

void RenderPipelineInvestigator::StoreIntermediateStats(RenderPassId pass, const HdrBufferStats& stats) {
    switch (pass) {
        case RenderPassId::PostProcessing: m_CameraLog.hdrBeforeExposure = stats; break;
        case RenderPassId::Exposure: m_CameraLog.hdrAfterExposure = stats; break;
        case RenderPassId::ToneMapping: m_CameraLog.hdrBeforeToneMap = stats; m_CameraLog.finalLdr = stats; break;
        default: break;
    }
}

PipelineInvestigationReport RenderPipelineInvestigator::FinalizeFrame(const VulkanContext& context, VkFence fence) {
    PipelineInvestigationReport report = m_LastReport;
    report.cameraLog = m_CameraLog;
    report.audit = m_Audit;

    if (!m_Settings.enabled || m_WarmupRemaining > 0) {
        DestroyPending(context);
        m_Pending.clear();
        return report;
    }

    if (!m_Settings.enableGpuReadback) {
        DestroyPending(context);
        m_Pending.clear();
        return report;
    }

    if (fence != VK_NULL_HANDLE) vkWaitForFences(context.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    for (const auto& cp : m_Pending) {
        PassCheckpointResult result{};
        result.pass = cp.pass;
        result.stats = ReadbackStaging(context, cp);
        const bool isLdr = (cp.pass == RenderPassId::ToneMapping || cp.pass == RenderPassId::Present);
        result.failed = !ValidateStats(result.stats, isLdr, result.failureReason);
        report.checkpoints.push_back(result);
        StoreIntermediateStats(cp.pass, result.stats);

        if (result.failed && !report.frameFailed) {
            report.frameFailed = true;
            report.firstFailingPass = cp.pass;
            report.firstFailureReason = result.failureReason;
        }
    }

    if (m_Audit.skyPassCount > 1) {
        report.frameFailed = true;
        if (report.firstFailingPass == RenderPassId::Count) {
            report.firstFailingPass = RenderPassId::SkyAtmosphere;
            report.firstFailureReason = "SkyAtmosphere executed " + std::to_string(m_Audit.skyPassCount) + " times";
            report.minimalFixHint = "Ensure DrawSkyAtmospherePass is called exactly once per frame.";
        }
    }
    if (m_Audit.exposurePassCount > 1 || m_Audit.toneMapPassCount > 1) {
        report.frameFailed = true;
        if (report.firstFailingPass == RenderPassId::Count) {
            report.firstFailingPass = RenderPassId::PostProcessing;
            report.firstFailureReason = "Post exposure/tone map executed more than once";
            report.minimalFixHint = "PostProcessStack::Apply must run exactly once per frame.";
        }
    }

    m_LastReport = report;
    if (m_Settings.logEveryFrame) LogFrameReport(report);
    if (m_Settings.writeReportFile) WriteReportFile(report);
    if (report.frameFailed && m_Settings.haltOnInvalid) m_ShouldHalt = true;

    DestroyPending(context);
    m_Pending.clear();
    return report;
}

void RenderPipelineInvestigator::LogFrameReport(const PipelineInvestigationReport& report) const {
    std::ostringstream ss;
    ss << "PIPELINE frame=" << report.frameIndex
       << " cam=(" << report.cameraLog.cameraX << "," << report.cameraLog.cameraY << "," << report.cameraLog.cameraZ << ")"
       << " height=" << report.cameraLog.cameraHeight
       << " EV=" << report.cameraLog.exposureEV
       << " hdrSkyLum=" << report.cameraLog.hdrSkyLuminance
       << " gpuAvgLum=" << report.cameraLog.avgLuminanceGpu;
    WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
    for (const auto& cp : report.checkpoints) {
        std::ostringstream line;
        line << "  [" << PassName(cp.pass) << "]"
             << " min=(" << cp.stats.minR << "," << cp.stats.minG << "," << cp.stats.minB << ")"
             << " max=(" << cp.stats.maxR << "," << cp.stats.maxG << "," << cp.stats.maxB << ")"
             << " avgLum=" << cp.stats.avgLuminance
             << " nan=" << cp.stats.nanCount << " inf=" << cp.stats.infCount
             << " neg=" << cp.stats.negativeCount << " over=" << cp.stats.overLimitCount;
        if (cp.failed) line << " FAIL: " << cp.failureReason;
        WE_LOG_INFO(LogCategory::Renderer.data(), line.str());
    }
    if (report.frameFailed) {
        WE_LOG_CRITICAL(LogCategory::Renderer.data(),
            std::string("PIPELINE FIRST FAIL [") + PassName(report.firstFailingPass) + "]: "
                + report.firstFailureReason + (report.minimalFixHint.empty() ? "" : " | fix: " + report.minimalFixHint));
    }
}

void RenderPipelineInvestigator::WriteReportFile(const PipelineInvestigationReport& report) const {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "PipelineInvestigation";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / ("frame_" + std::to_string(report.frameIndex) + ".txt");
    std::ofstream file(path);
    if (!file) return;
    file << "frame=" << report.frameIndex << "\n";
    file << "failed=" << (report.frameFailed ? "yes" : "no") << "\n";
    if (report.frameFailed) {
        file << "first_fail_pass=" << PassName(report.firstFailingPass) << "\n";
        file << "reason=" << report.firstFailureReason << "\n";
        file << "fix=" << report.minimalFixHint << "\n";
    }
    for (const auto& cp : report.checkpoints) {
        file << PassName(cp.pass) << " max=(" << cp.stats.maxR << "," << cp.stats.maxG << "," << cp.stats.maxB << ")"
             << " avgLum=" << cp.stats.avgLuminance << " fail=" << (cp.failed ? cp.failureReason : "no") << "\n";
    }
}

#endif
} // namespace we::runtime::renderer
