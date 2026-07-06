#include "Renderer/RenderForensics.hpp"
#include "Core/LogCategory.hpp"
#include "Core/DiagnosticMacros.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#endif

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

ForensicPixelRGBA SampleAt(const std::vector<float>& rgba, uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
    const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
    if (idx + 2 >= rgba.size()) return {};
    ForensicPixelRGBA px{};
    px.r = rgba[idx];
    px.g = rgba[idx + 1];
    px.b = rgba[idx + 2];
    px.a = rgba[idx + 3];
    return px;
}

void AppendPixelLog(std::ostringstream& ss, const char* label, const ForensicPixelRGBA& px) {
    ss << label << ":\n"
       << "  R: " << px.r << "\n"
       << "  G: " << px.g << "\n"
       << "  B: " << px.b << "\n"
       << "  A: " << px.a << "\n";
}

bool IsAttachmentLayout(ForensicPassId pass) {
    switch (pass) {
        case ForensicPassId::Clear:
        case ForensicPassId::Shadow:
        case ForensicPassId::Geometry:
        case ForensicPassId::Lighting:
        case ForensicPassId::SkyAtmosphere:
        case ForensicPassId::VolumetricClouds:
        case ForensicPassId::Fog:
        case ForensicPassId::AerialPerspective:
        case ForensicPassId::Grid:
        case ForensicPassId::Gizmos:
            return true;
        default:
            return false;
    }
}

const char* HealthIndicator(ForensicHealth health) {
    switch (health) {
        case ForensicHealth::Pass: return "PASS";
        case ForensicHealth::Warning: return "WARNING";
        case ForensicHealth::Error: return "ERROR";
        default: return "?";
    }
}

std::string FmtRgb(float r, float g, float b) {
    std::ostringstream ss;
    ss << "(" << r << "," << g << "," << b << ")";
    return ss.str();
}

bool IsHdrStatsValid(const ForensicTargetStats& stats, float maxComponent) {
    if (!stats.valid) return true;
    if (stats.nanCount > 0 || stats.infCount > 0) return false;
    if (stats.maxR > maxComponent || stats.maxG > maxComponent || stats.maxB > maxComponent) return false;
    return true;
}

bool IsLdrStatsValid(const ForensicTargetStats& stats, float whitePercentLimit, float whiteThreshold) {
    if (!stats.valid) return true;
    if (stats.nanCount > 0 || stats.infCount > 0) return false;
    if (stats.maxR > 1.05f || stats.maxG > 1.05f || stats.maxB > 1.05f) return false;
    if (stats.whitePixelPercent >= whitePercentLimit) return false;
    return true;
}

} // namespace

RenderForensics& RenderForensics::Get() {
    static RenderForensics instance;
    return instance;
}

RenderForensicsSettings RenderForensics::DefaultEditorSettings() {
    RenderForensicsSettings settings{};
    // Opt-in only: per-pass GPU readback stalls the editor and can fatal-exit on layout mismatches.
    settings.enabled = false;
    settings.enableGpuReadback = false;
    settings.logEveryFrame = false;
    settings.writeReportFile = true;
    settings.haltOnInvalid = false;
    settings.haltOnWhiteScreen = false;
    settings.warmupFrames = 2;
    settings.sampleStride = 8;
    settings.hdrWhitePixelThreshold = 64.0f;
    settings.ldrWhitePixelThreshold = 0.99f;
    settings.whiteScreenPercent = 95.0f;
    settings.maxRgbJumpRatio = 2.0f;
    settings.maxCallStackFrames = 24;
    return settings;
}

bool RenderForensics::IsLdrPass(ForensicPassId pass) {
    return pass == ForensicPassId::ToneMapping
        || pass == ForensicPassId::UI
        || pass == ForensicPassId::Present;
}

const char* RenderForensics::PassName(ForensicPassId pass) {
    switch (pass) {
        case ForensicPassId::Clear: return "Clear";
        case ForensicPassId::Shadow: return "Shadow";
        case ForensicPassId::AtmosphereLUT: return "AtmosphereLUT";
        case ForensicPassId::Geometry: return "Geometry";
        case ForensicPassId::Lighting: return "Lighting";
        case ForensicPassId::SkyAtmosphere: return "SkyAtmosphere";
        case ForensicPassId::VolumetricClouds: return "VolumetricClouds";
        case ForensicPassId::Fog: return "Fog";
        case ForensicPassId::AerialPerspective: return "AerialPerspective";
        case ForensicPassId::Grid: return "Grid";
        case ForensicPassId::Gizmos: return "Gizmos";
        case ForensicPassId::SceneComposite: return "SceneComposite";
        case ForensicPassId::LuminanceReduce: return "LuminanceReduce";
        case ForensicPassId::Bloom: return "Bloom";
        case ForensicPassId::Exposure: return "Exposure";
        case ForensicPassId::ToneMapping: return "ToneMapping";
        case ForensicPassId::UI: return "UI";
        case ForensicPassId::Present: return "Present";
        default: return "Unknown";
    }
}

void RenderForensics::Configure(const RenderForensicsSettings& settings) {
    m_Settings = settings;
    m_WarmupRemaining = settings.warmupFrames;
    m_ShouldHalt = false;
    if (!settings.enabled) return;
    WE_LOG_INFO(LogCategory::Renderer.data(),
        "Render pipeline debugger enabled (readback=" + std::string(settings.enableGpuReadback ? "on" : "off")
            + ", haltOnInvalid=" + (settings.haltOnInvalid ? "on" : "off") + ").");
}

void RenderForensics::BeginFrame(uint64_t frameIndex) {
    for (auto& pending : m_Pending) {
        if (s_CachedDevice != VK_NULL_HANDLE) {
            if (pending.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(s_CachedDevice, pending.stagingBuffer, nullptr);
            if (pending.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(s_CachedDevice, pending.stagingMemory, nullptr);
        }
    }
    m_Pending.clear();
    m_Executions.clear();
    m_NextGlobalOrder = 0;
    m_Audit = {};
    m_LastReport = {};
    m_LastReport.frameIndex = frameIndex;
    if (m_WarmupRemaining > 0) --m_WarmupRemaining;
}

void RenderForensics::DestroyPending(const VulkanContext& context) {
    const VkDevice device = context.GetDevice();
    if (device != VK_NULL_HANDLE) s_CachedDevice = device;
    for (auto& pending : m_Pending) {
        if (device == VK_NULL_HANDLE) continue;
        if (pending.stagingBuffer != VK_NULL_HANDLE) vkDestroyBuffer(device, pending.stagingBuffer, nullptr);
        if (pending.stagingMemory != VK_NULL_HANDLE) vkFreeMemory(device, pending.stagingMemory, nullptr);
        pending.stagingBuffer = VK_NULL_HANDLE;
        pending.stagingMemory = VK_NULL_HANDLE;
    }
}

ForensicPassExecution& RenderForensics::ExecutionAt(int index) {
    if (index < 0 || index >= static_cast<int>(m_Executions.size())) {
        static ForensicPassExecution s_Empty{};
        return s_Empty;
    }
    return m_Executions[static_cast<size_t>(index)];
}

const ForensicPassExecution& RenderForensics::ExecutionAt(int index) const {
    return const_cast<RenderForensics*>(this)->ExecutionAt(index);
}

std::vector<std::string> RenderForensics::CaptureCallStack(int maxFrames) const {
    std::vector<std::string> frames;
    if (maxFrames <= 0) return frames;

#if defined(_WIN32)
    void* addresses[64]{};
    const USHORT captured = CaptureStackBackTrace(2, static_cast<DWORD>((std::min)(maxFrames, 64)), addresses, nullptr);
    HANDLE process = GetCurrentProcess();
    static bool symInit = false;
    if (!symInit) {
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        symInit = SymInitialize(process, nullptr, TRUE) == TRUE;
    }

    alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME]{};
    auto* symbol = reinterpret_cast<PSYMBOL_INFO>(symbolBuffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    for (USHORT i = 0; i < captured; ++i) {
        DWORD64 displacement = 0;
        std::ostringstream line;
        if (symInit && SymFromAddr(process, reinterpret_cast<DWORD64>(addresses[i]), &displacement, symbol)) {
            line << symbol->Name;
        } else {
            line << "0x" << std::hex << reinterpret_cast<uintptr_t>(addresses[i]);
        }

        IMAGEHLP_LINE64 lineInfo{};
        lineInfo.SizeOfStruct = sizeof(lineInfo);
        DWORD lineDisplacement = 0;
        if (symInit && SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(addresses[i]), &lineDisplacement, &lineInfo)) {
            line << " at " << lineInfo.FileName << ":" << lineInfo.LineNumber;
        }
        frames.push_back(line.str());
    }
#else
    (void)maxFrames;
    frames.push_back("(call stack capture unavailable on this platform)");
#endif
    return frames;
}

void RenderForensics::RecordCameraAndEnvironment(
    const glm::vec3& cameraPos,
    const glm::vec3& cameraForward,
    const SceneEnvironmentUniform& env,
    float gpuAvgLuminance) {

    m_CameraLog.cameraX = cameraPos.x;
    m_CameraLog.cameraY = cameraPos.y;
    m_CameraLog.cameraZ = cameraPos.z;
    m_CameraLog.cameraDirX = cameraForward.x;
    m_CameraLog.cameraDirY = cameraForward.y;
    m_CameraLog.cameraDirZ = cameraForward.z;
    m_CameraLog.cameraHeight = cameraPos.y - env.worldOrigin.y;
    m_CameraLog.planetRadius = env.planetRadius;
    m_CameraLog.atmosphereRadius = env.planetRadius + env.atmosphereHeight;
    m_CameraLog.sunDirX = env.sunDirection.x;
    m_CameraLog.sunDirY = env.sunDirection.y;
    m_CameraLog.sunDirZ = env.sunDirection.z;
    m_CameraLog.sunColorR = env.sunColor.x;
    m_CameraLog.sunColorG = env.sunColor.y;
    m_CameraLog.sunColorB = env.sunColor.z;
    m_CameraLog.sunIntensity = env.sunIntensity;
    m_CameraLog.exposureEV = env.exposureEV;
    m_CameraLog.ev100 = env.exposureEV;
    m_CameraLog.exposureMultiplier = std::pow(2.0f, -env.exposureEV);
    m_CameraLog.avgSceneLuminance = gpuAvgLuminance;
}

int RenderForensics::BeginPassExecution(
    ForensicPassId pass,
    ForensicPassMetadata metadata,
    const char* sourceFile,
    int sourceLine) {

    if (!m_Settings.enabled) return -1;

    ForensicPassExecution exec{};
    exec.globalOrder = ++m_NextGlobalOrder;
    exec.passId = pass;
    exec.passName = PassName(pass);
    exec.executed = true;
    exec.metadata = std::move(metadata);
    if (sourceFile != nullptr) {
        exec.metadata.sourceFile = sourceFile;
        exec.metadata.sourceLine = sourceLine;
    }
    exec.callStack = CaptureCallStack(m_Settings.maxCallStackFrames);

    const int passIndex = static_cast<int>(pass);
    if (passIndex >= 0 && passIndex < static_cast<int>(ForensicPassId::Count)) {
        exec.executionNumber = ++m_Audit.passCounts[static_cast<size_t>(passIndex)];
    }

    if (!m_Executions.empty()) {
        const auto& prev = m_Executions.back();
        if (prev.outputStats.valid) {
            exec.inputStats = prev.outputStats;
        }
    }

    const int executionIndex = static_cast<int>(m_Executions.size());
    m_Executions.push_back(std::move(exec));
    return executionIndex;
}

void RenderForensics::EndPassExecution(int executionIndex, double cpuMs, bool succeeded) {
    if (!m_Settings.enabled || executionIndex < 0) return;
    auto& exec = ExecutionAt(executionIndex);
    exec.cpuMs = cpuMs;
    exec.gpuMs = cpuMs;
    exec.succeeded = succeeded;
    exec.executed = true;
}

void RenderForensics::RecordExposureDetails(
    int executionIndex,
    float ev100,
    float exposureMultiplier,
    float avgSceneLuminance) {

    if (!m_Settings.enabled || executionIndex < 0) return;
    auto& exec = ExecutionAt(executionIndex);
    exec.hasExposureDetails = true;
    exec.exposureDetails.executionNumber = exec.executionNumber;
    exec.exposureDetails.ev100 = ev100;
    exec.exposureDetails.exposureMultiplier = exposureMultiplier;
    exec.exposureDetails.avgSceneLuminance = avgSceneLuminance;
    exec.exposureDetails.callStack = exec.callStack;
    if (exec.inputStats.valid) {
        exec.exposureDetails.inputHdr = exec.inputStats;
    }
}

void RenderForensics::AuditPassBegin(ForensicPassId pass) {
    ForensicPassMetadata meta{};
    BeginPassExecution(pass, meta, nullptr, 0);
}

void RenderForensics::RecordPassMetadata(ForensicPassId pass, ForensicPassMetadata metadata) {
    if (!m_Settings.enabled || m_Executions.empty()) return;
    for (auto it = m_Executions.rbegin(); it != m_Executions.rend(); ++it) {
        if (it->passId == pass) {
            it->metadata = std::move(metadata);
            return;
        }
    }
    BeginPassExecution(pass, std::move(metadata), nullptr, 0);
}

void RenderForensics::RecordPassTiming(ForensicPassId pass, double cpuMs, bool succeeded) {
    if (!m_Settings.enabled || m_Executions.empty()) return;
    for (auto it = m_Executions.rbegin(); it != m_Executions.rend(); ++it) {
        if (it->passId == pass) {
            it->cpuMs = cpuMs;
            it->gpuMs = cpuMs;
            it->succeeded = succeeded;
            it->executed = true;
            return;
        }
    }
}

void RenderForensics::EnqueueReadback(
    VkCommandBuffer cmd,
    const VulkanContext& context,
    VkImage colorImage,
    uint32_t width,
    uint32_t height,
    ForensicPassId pass,
    VkImageLayout currentLayout,
    int executionIndex) {

    if (!IsReadbackEnabled() || m_WarmupRemaining > 0 || colorImage == VK_NULL_HANDLE || width == 0 || height == 0) {
        return;
    }

    if (executionIndex < 0) {
        for (auto it = m_Executions.rbegin(); it != m_Executions.rend(); ++it) {
            if (it->passId == pass) {
                executionIndex = static_cast<int>(m_Executions.rend() - it - 1);
                break;
            }
        }
    }

    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * height;

    s_CachedDevice = context.GetDevice();

    PendingReadback pending{};
    pending.executionIndex = executionIndex;
    pending.pass = pass;
    pending.width = width;
    pending.height = height;
    context.CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        pending.stagingBuffer,
        pending.stagingMemory);

    VkImageLayout srcLayout = currentLayout;
    if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        srcLayout = IsAttachmentLayout(pass)
            ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    context.CmdTransitionImageLayout(cmd, colorImage, srcLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { width, height, 1 };
    vkCmdCopyImageToBuffer(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pending.stagingBuffer, 1, &region);

    context.CmdTransitionImageLayout(cmd, colorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcLayout);

    m_Pending.push_back(pending);
}

ForensicTargetStats RenderForensics::AnalyzePixels(
    const std::vector<float>& rgba,
    uint32_t width,
    uint32_t height,
    bool isLdr) const {

    ForensicTargetStats stats{};
    if (rgba.empty() || width == 0 || height == 0) return stats;

    const int stride = (std::max)(1, m_Settings.sampleStride);
    stats.minR = stats.minG = stats.minB = 1e30f;
    stats.maxR = stats.maxG = stats.maxB = -1e30f;
    double sumR = 0.0, sumG = 0.0, sumB = 0.0, lumSum = 0.0;
    uint32_t whiteCount = 0, blackCount = 0;
    float firstR = 0.0f, firstG = 0.0f, firstB = 0.0f;
    bool firstSample = true;
    bool uniform = true;

    const float whiteThreshold = isLdr ? m_Settings.ldrWhitePixelThreshold : m_Settings.hdrWhitePixelThreshold;

    for (uint32_t y = 0; y < height; y += static_cast<uint32_t>(stride)) {
        for (uint32_t x = 0; x < width; x += static_cast<uint32_t>(stride)) {
            const size_t idx = (static_cast<size_t>(y) * width + x) * 4;
            if (idx + 2 >= rgba.size()) continue;
            const float r = rgba[idx];
            const float g = rgba[idx + 1];
            const float b = rgba[idx + 2];
            ++stats.samples;

            if (std::isnan(r) || std::isnan(g) || std::isnan(b)) ++stats.nanCount;
            if (std::isinf(r) || std::isinf(g) || std::isinf(b)) ++stats.infCount;
            if (r > 10.0f || g > 10.0f || b > 10.0f) ++stats.over10Count;
            if (r > 100.0f || g > 100.0f || b > 100.0f) ++stats.over100Count;

            stats.minR = (std::min)(stats.minR, r); stats.minG = (std::min)(stats.minG, g); stats.minB = (std::min)(stats.minB, b);
            stats.maxR = (std::max)(stats.maxR, r); stats.maxG = (std::max)(stats.maxG, g); stats.maxB = (std::max)(stats.maxB, b);
            sumR += r; sumG += g; sumB += b;
            lumSum += 0.2126 * r + 0.7152 * g + 0.0722 * b;

            if (r > whiteThreshold && g > whiteThreshold && b > whiteThreshold) {
                ++whiteCount;
            }
            if (r < m_Settings.blackPixelThreshold && g < m_Settings.blackPixelThreshold && b < m_Settings.blackPixelThreshold) {
                ++blackCount;
            }

            if (firstSample) {
                firstR = r; firstG = g; firstB = b;
                firstSample = false;
            } else if (uniform) {
                const float eps = 1e-4f;
                if (std::fabs(r - firstR) > eps || std::fabs(g - firstG) > eps || std::fabs(b - firstB) > eps) {
                    uniform = false;
                }
            }
        }
    }

    if (stats.samples > 0) {
        const double n = static_cast<double>(stats.samples);
        stats.avgR = static_cast<float>(sumR / n);
        stats.avgG = static_cast<float>(sumG / n);
        stats.avgB = static_cast<float>(sumB / n);
        stats.avgLuminance = static_cast<float>(lumSum / n);
        stats.whitePixelPercent = 100.0f * static_cast<float>(whiteCount) / static_cast<float>(stats.samples);
        stats.blackPixelPercent = 100.0f * static_cast<float>(blackCount) / static_cast<float>(stats.samples);
        stats.allPixelsSameColor = uniform;
        stats.valid = true;
    }

    const uint32_t cx = width / 2;
    const uint32_t cy = height / 2;
    stats.corners.center = SampleAt(rgba, width, height, cx, cy);
    stats.corners.topLeft = SampleAt(rgba, width, height, 0, 0);
    stats.corners.topRight = SampleAt(rgba, width, height, width - 1, 0);
    stats.corners.bottomLeft = SampleAt(rgba, width, height, 0, height - 1);
    stats.corners.bottomRight = SampleAt(rgba, width, height, width - 1, height - 1);

    return stats;
}

ForensicTargetStats RenderForensics::ReadbackStaging(const VulkanContext& context, const PendingReadback& pending) const {
    ForensicTargetStats stats{};
    if (pending.stagingBuffer == VK_NULL_HANDLE) return stats;

    const VkDeviceSize bytesPerPixel = 8;
    const VkDeviceSize rowPitch = ((pending.width * bytesPerPixel + 255) / 256) * 256;
    const VkDeviceSize bufferSize = rowPitch * pending.height;

    void* mapped = nullptr;
    vkMapMemory(context.GetDevice(), pending.stagingMemory, 0, bufferSize, 0, &mapped);

    std::vector<float> rgba(static_cast<size_t>(pending.width) * pending.height * 4);
    for (uint32_t y = 0; y < pending.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint8_t*>(mapped) + y * rowPitch;
        for (uint32_t x = 0; x < pending.width; ++x) {
            const auto* pixel = row + x * bytesPerPixel;
            const auto r16 = static_cast<std::uint16_t>(pixel[0] | (pixel[1] << 8));
            const auto g16 = static_cast<std::uint16_t>(pixel[2] | (pixel[3] << 8));
            const auto b16 = static_cast<std::uint16_t>(pixel[4] | (pixel[5] << 8));
            const size_t idx = (static_cast<size_t>(y) * pending.width + x) * 4;
            rgba[idx] = HalfToFloat(r16);
            rgba[idx + 1] = HalfToFloat(g16);
            rgba[idx + 2] = HalfToFloat(b16);
            rgba[idx + 3] = 1.0f;
        }
    }
    vkUnmapMemory(context.GetDevice(), pending.stagingMemory);

    return AnalyzePixels(rgba, pending.width, pending.height, IsLdrPass(pending.pass));
}

ForensicPassDelta RenderForensics::ComputeDelta(const ForensicTargetStats& before, const ForensicTargetStats& after) const {
    ForensicPassDelta delta{};
    if (!before.valid || !after.valid) return delta;
    delta.valid = true;
    delta.avgRBefore = before.avgR;
    delta.avgGBefore = before.avgG;
    delta.avgBBefore = before.avgB;
    delta.avgRAfter = after.avgR;
    delta.avgGAfter = after.avgG;
    delta.avgBAfter = after.avgB;
    delta.avgRDiff = after.avgR - before.avgR;
    delta.avgGDiff = after.avgG - before.avgG;
    delta.avgBDiff = after.avgB - before.avgB;
    delta.whitePercentBefore = before.whitePixelPercent;
    delta.whitePercentAfter = after.whitePixelPercent;
    delta.centerBefore = before.corners.center;
    delta.centerAfter = after.corners.center;

    auto ratioExceeds = [&](float prev, float next) {
        const float absPrev = (std::max)(std::fabs(prev), 1e-4f);
        const float absNext = (std::max)(std::fabs(next), 1e-4f);
        const float ratio = (std::max)(absNext / absPrev, absPrev / absNext);
        return ratio > m_Settings.maxRgbJumpRatio;
    };
    delta.rgbJumpExceeds2x = ratioExceeds(before.avgR, after.avgR)
        || ratioExceeds(before.avgG, after.avgG)
        || ratioExceeds(before.avgB, after.avgB);
    return delta;
}

ForensicHealth RenderForensics::ClassifyPassHealth(const ForensicPassExecution& record, bool isLdr) const {
    if (!record.succeeded) return ForensicHealth::Error;
    if (!record.validationErrors.empty()) {
        for (const auto& err : record.validationErrors) {
            if (err.find("NaN") != std::string::npos || err.find("Inf") != std::string::npos
                || err.find("completely white") != std::string::npos
                || err.find("duplicate execution") != std::string::npos
                || err.find("LDR output") != std::string::npos) {
                return ForensicHealth::Error;
            }
        }
        return ForensicHealth::Warning;
    }
    const auto& s = record.outputStats;
    if (!s.valid) return record.executed ? ForensicHealth::Warning : ForensicHealth::Pass;
    if (s.nanCount > 0 || s.infCount > 0) return ForensicHealth::Error;
    if (!isLdr && s.over100Count > 0) return ForensicHealth::Warning;
    if (isLdr && s.whitePixelPercent >= m_Settings.whiteScreenPercent) return ForensicHealth::Error;
    return ForensicHealth::Pass;
}

void RenderForensics::ValidatePassExecution(ForensicPassExecution& record, bool isLdr) {
    const auto& s = record.outputStats;
    if (!s.valid) return;

    if (s.nanCount > 0) {
        record.validationErrors.push_back("Shader output contains NaN (" + std::to_string(s.nanCount) + " sampled pixels)");
    }
    if (s.infCount > 0) {
        record.validationErrors.push_back("Shader output contains Inf (" + std::to_string(s.infCount) + " sampled pixels)");
    }

    if (!isLdr) {
        if (s.over100Count > 0) {
            record.validationErrors.push_back("HDR color exceeds limit " + std::to_string(m_Settings.maxHdrComponent)
                + " (" + std::to_string(s.over100Count) + " sampled pixels)");
        }
        if (s.maxR > m_Settings.maxHdrComponent || s.maxG > m_Settings.maxHdrComponent || s.maxB > m_Settings.maxHdrComponent) {
            record.validationErrors.push_back("HDR max RGB exceeds limit: "
                + FmtRgb(s.maxR, s.maxG, s.maxB));
        }
    } else {
        if (s.maxR > 1.05f || s.maxG > 1.05f || s.maxB > 1.05f) {
            record.validationErrors.push_back("LDR output still in HDR range after tone mapping: max="
                + FmtRgb(s.maxR, s.maxG, s.maxB));
        }
        if (s.whitePixelPercent >= m_Settings.whiteScreenPercent) {
            record.validationErrors.push_back("Render target is completely white ("
                + std::to_string(s.whitePixelPercent) + "% pixels > " + std::to_string(m_Settings.ldrWhitePixelThreshold) + ")");
        }
    }

    if (record.delta.valid && record.delta.rgbJumpExceeds2x
        && record.passId != ForensicPassId::Clear
        && record.passId != ForensicPassId::SkyAtmosphere) {
        record.validationErrors.push_back("Average RGB changed by more than "
            + std::to_string(m_Settings.maxRgbJumpRatio) + "x vs previous pass");
    }

    if (record.passId == ForensicPassId::Exposure && record.hasExposureDetails) {
        const auto& e = record.exposureDetails;
        if (!std::isfinite(e.exposureMultiplier) || e.exposureMultiplier <= 0.0f) {
            record.validationErrors.push_back("Exposure multiplier is invalid: " + std::to_string(e.exposureMultiplier));
        }
    }
}

void RenderForensics::DetectAnomalies(ForensicFrameReport& report) {
    for (auto& exec : report.passes) {
        const int passIndex = static_cast<int>(exec.passId);
        if (passIndex >= 0 && passIndex < static_cast<int>(ForensicPassId::Count)) {
            const int count = report.audit.passCounts[static_cast<size_t>(passIndex)];
            const bool allowMultiple = exec.passId == ForensicPassId::Bloom;
            if (!allowMultiple && count > 1 && exec.executionNumber > 1) {
                exec.validationErrors.push_back("duplicate execution #" + std::to_string(exec.executionNumber)
                    + " for pass " + exec.passName);
                if (!report.frameFailed) {
                    report.frameFailed = true;
                    report.firstAnomalyPass = exec.passId;
                    report.firstAnomalyReason = exec.passName + " executed " + std::to_string(count) + " times";
                    report.rootCauseFile = exec.metadata.sourceFile;
                    report.rootCauseLine = exec.metadata.sourceLine;
                }
            }
        }

        const bool isLdr = IsLdrPass(exec.passId);
        if (!exec.outputStats.valid) continue;

        if (!isLdr && exec.outputStats.whitePixelPercent >= m_Settings.whiteScreenPercent) {
            // HDR can be bright; only flag if physically impossible.
            if (exec.outputStats.over100Count > 0 || exec.outputStats.nanCount > 0 || exec.outputStats.infCount > 0) {
                if (!report.frameFailed) {
                    report.frameFailed = true;
                    report.firstAnomalyPass = exec.passId;
                    report.firstAnomalyReason = "HDR pass produced out-of-range values";
                    report.rootCauseFile = exec.metadata.sourceFile;
                    report.rootCauseLine = exec.metadata.sourceLine;
                }
            }
        }

        if (isLdr && exec.outputStats.whitePixelPercent >= m_Settings.whiteScreenPercent) {
            report.frameFailed = true;
            report.haltedForWhiteScreen = true;
            if (report.firstAnomalyPass == ForensicPassId::Count) {
                report.firstAnomalyPass = exec.passId;
                report.firstAnomalyReason = "First LDR pass where output became predominantly white at '" + exec.passName + "'";
                report.rootCauseFile = exec.metadata.sourceFile;
                report.rootCauseLine = exec.metadata.sourceLine;
                report.rootCauseExplanation = "Pass '" + exec.passName + "' produced "
                    + std::to_string(exec.outputStats.whitePixelPercent) + "% white pixels.";
            }
            exec.health = ForensicHealth::Error;
        }
    }
}

void RenderForensics::BuildInvestigationConclusion(ForensicFrameReport& report) {
    auto& c = report.conclusion;
    ForensicTargetStats preExposure{};
    bool hasPreExposure = false;

    for (const auto& exec : report.passes) {
        const bool isLdr = IsLdrPass(exec.passId);
        if (exec.passId == ForensicPassId::SceneComposite && exec.outputStats.valid) {
            preExposure = exec.outputStats;
            hasPreExposure = true;
        }
        if (exec.passId == ForensicPassId::Exposure && exec.hasExposureDetails && exec.exposureDetails.inputHdr.valid) {
            preExposure = exec.exposureDetails.inputHdr;
            hasPreExposure = true;
        }

        if (!exec.outputStats.valid) continue;

        const bool hdrValid = IsHdrStatsValid(exec.outputStats, m_Settings.maxHdrComponent);
        const bool ldrValid = IsLdrStatsValid(exec.outputStats, m_Settings.whiteScreenPercent, m_Settings.ldrWhitePixelThreshold);
        const bool outputValid = isLdr ? ldrValid : hdrValid;

        if (!outputValid && c.firstInvalidOutputPass == ForensicPassId::Count) {
            c.firstInvalidOutputPass = exec.passId;
            if (!exec.validationErrors.empty()) {
                c.firstInvalidOutputReason = exec.validationErrors.front();
            } else if (isLdr) {
                c.firstInvalidOutputReason = "LDR output max=" + FmtRgb(exec.outputStats.maxR, exec.outputStats.maxG, exec.outputStats.maxB);
            } else {
                c.firstInvalidOutputReason = "HDR output out of range max=" + FmtRgb(exec.outputStats.maxR, exec.outputStats.maxG, exec.outputStats.maxB);
            }
            c.rootCauseFile = exec.metadata.sourceFile;
            c.rootCauseLine = exec.metadata.sourceLine;
            if (!exec.callStack.empty()) c.rootCauseFunction = exec.callStack.front();
        }
    }

    if (hasPreExposure) {
        c.framebufferInvalidBeforeExposure = !IsHdrStatsValid(preExposure, m_Settings.maxHdrComponent);
        report.cameraLog.hdrBeforeExposure = preExposure;
    }

    const int exposureCount = report.audit.passCounts[static_cast<size_t>(ForensicPassId::Exposure)];
    const int toneMapCount = report.audit.passCounts[static_cast<size_t>(ForensicPassId::ToneMapping)];

    if (exposureCount > 1 || toneMapCount > 1) {
        std::vector<const ForensicPassExecution*> exposureExecs;
        for (const auto& exec : report.passes) {
            if (exec.passId == ForensicPassId::Exposure) exposureExecs.push_back(&exec);
        }

        bool editorHook = false;
        bool postStackHook = false;
        for (const auto* exec : exposureExecs) {
            if (exec->metadata.sourceFile.find("Editor.cpp") != std::string::npos) editorHook = true;
            if (exec->metadata.sourceFile.find("PostProcessStack.cpp") != std::string::npos) postStackHook = true;
        }

        if (editorHook && postStackHook) {
            c.duplicateExposureIsInstrumentationArtifact = true;
            c.duplicateExposureAnalysis =
                "Exposure audited " + std::to_string(exposureCount) + " times due to duplicate forensic hooks in "
                "Editor.cpp and PostProcessStack.cpp. GPU PostCompositeCS dispatches once per frame.";
            c.minimalFix = "Remove duplicate RenderForensics::BeginPassExecution calls for Exposure/ToneMapping in Editor.cpp; "
                "keep instrumentation only inside PostProcessStack::Apply.";
        } else {
            c.duplicateExposureIsPrimaryCause = true;
            c.duplicateExposureAnalysis = "Exposure executed " + std::to_string(exposureCount)
                + " times from real render code paths.";
            if (!exposureExecs.empty()) {
                c.rootCauseFile = exposureExecs.back()->metadata.sourceFile;
                c.rootCauseLine = exposureExecs.back()->metadata.sourceLine;
                if (!exposureExecs.back()->callStack.empty()) {
                    c.rootCauseFunction = exposureExecs.back()->callStack.front();
                }
            }
            c.minimalFix = "Ensure PostProcessStack::Apply and any caller invoke exposure exactly once per frame.";
        }
    }

    if (c.firstInvalidOutputPass == ForensicPassId::ToneMapping
        || c.firstInvalidOutputPass == ForensicPassId::Exposure) {
        c.executiveSummary =
            "Framebuffer HDR content before exposure is "
            + std::string(c.framebufferInvalidBeforeExposure ? "INVALID" : "VALID")
            + ". PostComposite output remains in HDR range (max>1), indicating tone mapping did not produce display-ready LDR.";
        if (!c.minimalFix.empty()) c.minimalFix += " ";
        c.minimalFix += "Verify PostCompositeCS executes and writes sRGB values to the scene target.";
    } else if (c.firstInvalidOutputPass != ForensicPassId::Count) {
        c.executiveSummary = "First measurable invalid output at pass "
            + std::string(PassName(c.firstInvalidOutputPass)) + ": " + c.firstInvalidOutputReason + ".";
        if (c.duplicateExposureIsInstrumentationArtifact) {
            c.executiveSummary += " Duplicate exposure detection is a secondary instrumentation artifact, not the root cause.";
        }
    } else if (c.duplicateExposureIsInstrumentationArtifact) {
        c.executiveSummary = "No invalid framebuffer output detected. Duplicate exposure counts are instrumentation-only.";
        c.minimalFix = "Remove duplicate forensic AuditPassBegin hooks in Editor.cpp for Exposure/ToneMapping.";
    } else {
        c.executiveSummary = "All instrumented passes produced values within expected HDR/LDR limits.";
    }

    if (report.minimalFix.empty() && !c.minimalFix.empty()) {
        report.minimalFix = c.minimalFix;
    }
    if (report.rootCauseFile.empty() && !c.rootCauseFile.empty()) {
        report.rootCauseFile = c.rootCauseFile;
        report.rootCauseLine = c.rootCauseLine;
        report.rootCauseExplanation = c.executiveSummary;
    }
}

void RenderForensics::LogPassDelta(const ForensicPassExecution& record) const {
    if (!record.delta.valid) return;
    std::ostringstream ss;
    ss << "[FORENSIC] PASS DELTA: " << record.passName << "\n"
       << "Average RGB changed:\n"
       << "  Before: " << FmtRgb(record.delta.avgRBefore, record.delta.avgGBefore, record.delta.avgBBefore) << "\n"
       << "  After:  " << FmtRgb(record.delta.avgRAfter, record.delta.avgGAfter, record.delta.avgBAfter) << "\n"
       << "  Diff:   " << FmtRgb(record.delta.avgRDiff, record.delta.avgGDiff, record.delta.avgBDiff) << "\n"
       << "White Pixels:\n"
       << "  Before: " << record.delta.whitePercentBefore << "%\n"
       << "  After:  " << record.delta.whitePercentAfter << "%\n"
       << "Center Pixel:\n"
       << "  Before: " << FmtRgb(record.delta.centerBefore.r, record.delta.centerBefore.g, record.delta.centerBefore.b) << "\n"
       << "  After:  " << FmtRgb(record.delta.centerAfter.r, record.delta.centerAfter.g, record.delta.centerAfter.b);
    if (record.delta.rgbJumpExceeds2x) {
        ss << "\n  WARNING: RGB jump exceeds " << m_Settings.maxRgbJumpRatio << "x";
    }
    WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
}

void RenderForensics::LogPassExecution(const ForensicPassExecution& record) const {
    std::ostringstream ss;
    ss << "[FORENSIC] #" << record.globalOrder << " " << record.passName
       << " (exec #" << record.executionNumber << ")"
       << " [" << HealthIndicator(record.health) << "]"
       << " executed=" << (record.executed ? "yes" : "no")
       << " success=" << (record.succeeded ? "yes" : "no")
       << " cpuMs=" << record.cpuMs << " gpuMs=" << record.gpuMs;

    const auto& m = record.metadata;
    if (!m.outputTarget.empty()) {
        ss << "\n  inputRT=" << m.inputTarget << " outputRT=" << m.outputTarget
           << " " << m.width << "x" << m.height << " fmt=" << m.format
           << " inLayout=" << m.inputLayout << " outLayout=" << m.outputLayout
           << " clear=" << m.clearColor << " load=" << m.loadOp << " store=" << m.storeOp
           << " blend=" << m.blendState << " depth=" << m.depthState
           << " viewport=" << m.viewport << " scissor=" << m.scissor;
    }
    if (!m.pipelineName.empty()) {
        ss << "\n  pipeline=" << m.pipelineName
           << " vs=" << m.vertexShader << " ps=" << m.pixelShader << " cs=" << m.computeShader
           << " desc=" << m.descriptorSets << " push=" << m.pushConstants
           << " tex=" << m.boundTextures;
    }
    if (!m.sourceFile.empty()) {
        ss << "\n  source=" << m.sourceFile << ":" << m.sourceLine;
    }
    if (!record.callStack.empty()) {
        ss << "\n  callStack:";
        for (size_t i = 0; i < record.callStack.size(); ++i) {
            ss << "\n    [" << i << "] " << record.callStack[i];
        }
    }

    const auto& s = record.outputStats;
    if (s.valid) {
        ss << "\n  output min=" << FmtRgb(s.minR, s.minG, s.minB)
           << " max=" << FmtRgb(s.maxR, s.maxG, s.maxB)
           << " avg=" << FmtRgb(s.avgR, s.avgG, s.avgB)
           << " avgLum=" << s.avgLuminance
           << " nan=" << s.nanCount << " inf=" << s.infCount
           << " >10=" << s.over10Count << " >100=" << s.over100Count
           << " white%=" << s.whitePixelPercent << " black%=" << s.blackPixelPercent;
        AppendPixelLog(ss, "\n  Center Pixel", s.corners.center);
        AppendPixelLog(ss, "  Top Left", s.corners.topLeft);
        AppendPixelLog(ss, "  Top Right", s.corners.topRight);
        AppendPixelLog(ss, "  Bottom Left", s.corners.bottomLeft);
        AppendPixelLog(ss, "  Bottom Right", s.corners.bottomRight);
    }

    if (record.hasExposureDetails) {
        const auto& e = record.exposureDetails;
        ss << "\n  EXPOSURE exec #" << e.executionNumber
           << " EV100=" << e.ev100
           << " multiplier=" << e.exposureMultiplier
           << " avgSceneLum=" << e.avgSceneLuminance;
        if (e.inputHdr.valid) {
            ss << "\n  inputHDR avg=" << FmtRgb(e.inputHdr.avgR, e.inputHdr.avgG, e.inputHdr.avgB)
               << " max=" << FmtRgb(e.inputHdr.maxR, e.inputHdr.maxG, e.inputHdr.maxB);
        }
        if (e.outputHdr.valid) {
            ss << "\n  outputHDR avg=" << FmtRgb(e.outputHdr.avgR, e.outputHdr.avgG, e.outputHdr.avgB)
               << " max=" << FmtRgb(e.outputHdr.maxR, e.outputHdr.maxG, e.outputHdr.maxB);
        }
    }

    for (const auto& err : record.validationErrors) {
        ss << "\n  VALIDATION ERROR: " << err;
    }

    WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
    LogPassDelta(record);
}

void RenderForensics::LogCameraState(const ForensicCameraLog& log) const {
    std::ostringstream ss;
    ss << "[FORENSIC] Camera pos=(" << log.cameraX << "," << log.cameraY << "," << log.cameraZ << ")"
       << " dir=(" << log.cameraDirX << "," << log.cameraDirY << "," << log.cameraDirZ << ")"
       << " height=" << log.cameraHeight
       << " planetR=" << log.planetRadius << " atmosphereR=" << log.atmosphereRadius
       << " sunDir=(" << log.sunDirX << "," << log.sunDirY << "," << log.sunDirZ << ")"
       << " sunColor=(" << log.sunColorR << "," << log.sunColorG << "," << log.sunColorB << ")"
       << " sunIntensity=" << log.sunIntensity
       << " exposureEV=" << log.exposureEV
       << " multiplier=" << log.exposureMultiplier
       << " EV100=" << log.ev100
       << " avgSceneLum=" << log.avgSceneLuminance;
    WE_LOG_INFO(LogCategory::Renderer.data(), ss.str());
}

ForensicFrameReport RenderForensics::FinalizeFrame(const VulkanContext& context, VkFence fence) {
    ForensicFrameReport report = m_LastReport;
    report.cameraLog = m_CameraLog;
    report.audit = m_Audit;

    if (!m_Settings.enabled || m_WarmupRemaining > 0) {
        DestroyPending(context);
        m_Pending.clear();
        m_LastReport = report;
        return report;
    }

    if (m_Settings.enableGpuReadback && fence != VK_NULL_HANDLE) {
        vkWaitForFences(context.GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
    }

    for (const auto& pending : m_Pending) {
        if (pending.executionIndex < 0 || pending.executionIndex >= static_cast<int>(m_Executions.size())) {
            continue;
        }
        auto& exec = m_Executions[static_cast<size_t>(pending.executionIndex)];
        exec.outputStats = ReadbackStaging(context, pending);
        if (exec.inputStats.valid) {
            exec.delta = ComputeDelta(exec.inputStats, exec.outputStats);
        } else if (pending.executionIndex > 0) {
            const auto& prev = m_Executions[static_cast<size_t>(pending.executionIndex - 1)];
            if (prev.outputStats.valid) {
                exec.inputStats = prev.outputStats;
                exec.delta = ComputeDelta(prev.outputStats, exec.outputStats);
            }
        }

        const bool isLdr = IsLdrPass(exec.passId);
        ValidatePassExecution(exec, isLdr);
        exec.health = ClassifyPassHealth(exec, isLdr);

        if (exec.hasExposureDetails) {
            exec.exposureDetails.outputHdr = exec.outputStats;
        }

        switch (exec.passId) {
            case ForensicPassId::SceneComposite: report.cameraLog.hdrBeforeExposure = exec.outputStats; break;
            case ForensicPassId::Exposure:
                report.cameraLog.hdrBeforeExposure = exec.hasExposureDetails
                    ? exec.exposureDetails.inputHdr : exec.inputStats;
                report.cameraLog.hdrAfterExposure = exec.outputStats;
                break;
            case ForensicPassId::ToneMapping:
                report.cameraLog.hdrBeforeToneMap = exec.inputStats;
                report.cameraLog.finalLdr = exec.outputStats;
                break;
            default: break;
        }

        if (!exec.validationErrors.empty() && !report.frameFailed) {
            report.frameFailed = true;
            report.firstAnomalyPass = exec.passId;
            report.firstAnomalyReason = exec.validationErrors.front();
            report.rootCauseFile = exec.metadata.sourceFile;
            report.rootCauseLine = exec.metadata.sourceLine;
            report.rootCauseExplanation = exec.validationErrors.front();
        }
    }

    report.passes = m_Executions;
    DetectAnomalies(report);
    BuildInvestigationConclusion(report);

    for (auto& exec : report.passes) {
        if (exec.health == ForensicHealth::Error) report.frameHealth = ForensicHealth::Error;
        else if (exec.health == ForensicHealth::Warning && report.frameHealth != ForensicHealth::Error) {
            report.frameHealth = ForensicHealth::Warning;
        }
    }

    if (report.frameFailed) {
        if (report.frameHealth != ForensicHealth::Error) report.frameHealth = ForensicHealth::Error;
        if (m_Settings.haltOnInvalid || (m_Settings.haltOnWhiteScreen && report.haltedForWhiteScreen)) {
            m_ShouldHalt = true;
        }
    }

    m_LastReport = report;

    if (m_Settings.logEveryFrame) {
        LogCameraState(report.cameraLog);
        for (const auto& exec : report.passes) {
            LogPassExecution(exec);
        }
        LogFrameReport(report);
    }
    if (m_Settings.writeReportFile) {
        WriteReportFile(report);
        WriteFinalDiagnosticReport(report);
        if (report.haltedForWhiteScreen || report.frameFailed) WriteWhiteScreenReport(report);
    }

    DestroyPending(context);
    m_Pending.clear();
    return report;
}

void RenderForensics::LogFrameReport(const ForensicFrameReport& report) const {
    if (!report.frameFailed && report.conclusion.firstInvalidOutputPass == ForensicPassId::Count) return;
    std::ostringstream ss;
    ss << "[FORENSIC] FRAME " << report.frameIndex;
    if (report.frameFailed) {
        ss << " FAILED at pass " << PassName(report.firstAnomalyPass) << ": " << report.firstAnomalyReason;
    }
    if (!report.conclusion.executiveSummary.empty()) {
        ss << " | CONCLUSION: " << report.conclusion.executiveSummary;
    }
    if (!report.rootCauseFile.empty()) {
        ss << " | source=" << report.rootCauseFile << ":" << report.rootCauseLine;
    }
    if (!report.minimalFix.empty()) ss << " | fix: " << report.minimalFix;
    WE_LOG_CRITICAL(LogCategory::Renderer.data(), ss.str());
}

void RenderForensics::WriteReportFile(const ForensicFrameReport& report) const {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "RenderForensics";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / ("frame_" + std::to_string(report.frameIndex) + ".txt");
    std::ofstream file(path);
    if (!file) return;

    file << "frame=" << report.frameIndex << "\n";
    file << "health=" << HealthIndicator(report.frameHealth) << "\n";
    file << "failed=" << (report.frameFailed ? "yes" : "no") << "\n";
    if (report.frameFailed) {
        file << "first_anomaly_pass=" << PassName(report.firstAnomalyPass) << "\n";
        file << "reason=" << report.firstAnomalyReason << "\n";
        file << "source=" << report.rootCauseFile << ":" << report.rootCauseLine << "\n";
        file << "explanation=" << report.rootCauseExplanation << "\n";
        file << "minimal_fix=" << report.minimalFix << "\n";
    }

    const auto& c = report.cameraLog;
    file << "camera_pos=" << c.cameraX << "," << c.cameraY << "," << c.cameraZ << "\n";
    file << "exposureEV=" << c.exposureEV << " multiplier=" << c.exposureMultiplier
         << " ev100=" << c.ev100 << " avgLum=" << c.avgSceneLuminance << "\n";

    for (const auto& exec : report.passes) {
        file << "\n[" << exec.globalOrder << "] " << exec.passName
             << " exec#" << exec.executionNumber
             << " " << HealthIndicator(exec.health)
             << " cpuMs=" << exec.cpuMs << " gpuMs=" << exec.gpuMs << "\n";
        if (!exec.metadata.sourceFile.empty()) {
            file << "  source=" << exec.metadata.sourceFile << ":" << exec.metadata.sourceLine << "\n";
        }
        const auto& s = exec.outputStats;
        if (s.valid) {
            file << "  min=" << FmtRgb(s.minR, s.minG, s.minB)
                 << " max=" << FmtRgb(s.maxR, s.maxG, s.maxB)
                 << " avg=" << FmtRgb(s.avgR, s.avgG, s.avgB)
                 << " avgLum=" << s.avgLuminance
                 << " white%=" << s.whitePixelPercent << " black%=" << s.blackPixelPercent << "\n";
            file << "  center=" << FmtRgb(s.corners.center.r, s.corners.center.g, s.corners.center.b) << "\n";
        }
        if (exec.delta.valid) {
            file << "  delta avg before=" << FmtRgb(exec.delta.avgRBefore, exec.delta.avgGBefore, exec.delta.avgBBefore)
                 << " after=" << FmtRgb(exec.delta.avgRAfter, exec.delta.avgGAfter, exec.delta.avgBAfter) << "\n";
        }
        for (const auto& err : exec.validationErrors) {
            file << "  ERROR: " << err << "\n";
        }
        if (!exec.callStack.empty()) {
            file << "  callStack:\n";
            for (size_t i = 0; i < exec.callStack.size(); ++i) {
                file << "    [" << i << "] " << exec.callStack[i] << "\n";
            }
        }
    }
}

void RenderForensics::WriteFinalDiagnosticReport(const ForensicFrameReport& report) const {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "RenderForensics";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / "FINAL_DIAGNOSTIC_REPORT.txt";
    std::ofstream file(path);
    if (!file) return;

    const auto& c = report.conclusion;
    file << "=== RENDER PIPELINE FORENSIC REPORT ===\n";
    file << "Frame: " << report.frameIndex << "\n\n";

    file << "EXECUTIVE SUMMARY\n";
    file << (c.executiveSummary.empty() ? "(no conclusion)" : c.executiveSummary) << "\n\n";

    file << "FIRST INVALID OUTPUT PASS\n";
    file << "  Pass: " << (c.firstInvalidOutputPass == ForensicPassId::Count ? "none" : PassName(c.firstInvalidOutputPass)) << "\n";
    file << "  Reason: " << c.firstInvalidOutputReason << "\n";
    file << "  Source: " << c.rootCauseFile << ":" << c.rootCauseLine << "\n";
    file << "  Function: " << c.rootCauseFunction << "\n\n";

    file << "FRAMEBUFFER BEFORE EXPOSURE\n";
    file << "  Invalid before exposure: " << (c.framebufferInvalidBeforeExposure ? "YES" : "NO") << "\n";
    if (report.cameraLog.hdrBeforeExposure.valid) {
        const auto& h = report.cameraLog.hdrBeforeExposure;
        file << "  avg=" << FmtRgb(h.avgR, h.avgG, h.avgB)
             << " max=" << FmtRgb(h.maxR, h.maxG, h.maxB)
             << " avgLum=" << h.avgLuminance << "\n";
    }

    file << "\nDUPLICATE EXPOSURE ANALYSIS\n";
    file << "  Exposure execution count: " << report.audit.passCounts[static_cast<size_t>(ForensicPassId::Exposure)] << "\n";
    file << "  ToneMapping execution count: " << report.audit.passCounts[static_cast<size_t>(ForensicPassId::ToneMapping)] << "\n";
    file << "  Primary cause: " << (c.duplicateExposureIsPrimaryCause ? "YES" : "NO") << "\n";
    file << "  Instrumentation artifact: " << (c.duplicateExposureIsInstrumentationArtifact ? "YES" : "NO") << "\n";
    file << "  Analysis: " << c.duplicateExposureAnalysis << "\n\n";

    file << "MINIMAL FIX\n";
    file << "  " << (c.minimalFix.empty() ? report.minimalFix : c.minimalFix) << "\n\n";

    file << "PASS EXECUTION AUDIT\n";
    for (int i = 0; i < static_cast<int>(ForensicPassId::Count); ++i) {
        const int count = report.audit.passCounts[static_cast<size_t>(i)];
        if (count > 0) {
            file << "  " << PassName(static_cast<ForensicPassId>(i)) << ": " << count << "\n";
        }
    }

    file << "\nPASS-BY-PASS DELTAS\n";
    for (const auto& exec : report.passes) {
        if (!exec.delta.valid) continue;
        file << "  PASS: " << exec.passName << "\n";
        file << "    Average RGB Before: " << FmtRgb(exec.delta.avgRBefore, exec.delta.avgGBefore, exec.delta.avgBBefore) << "\n";
        file << "    Average RGB After:  " << FmtRgb(exec.delta.avgRAfter, exec.delta.avgGAfter, exec.delta.avgBAfter) << "\n";
        file << "    White% Before/After: " << exec.delta.whitePercentBefore << "% / " << exec.delta.whitePercentAfter << "%\n";
        file << "    Center Before: " << FmtRgb(exec.delta.centerBefore.r, exec.delta.centerBefore.g, exec.delta.centerBefore.b) << "\n";
        file << "    Center After:  " << FmtRgb(exec.delta.centerAfter.r, exec.delta.centerAfter.g, exec.delta.centerAfter.b) << "\n";
    }
}

void RenderForensics::WriteWhiteScreenReport(const ForensicFrameReport& report) const {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::path("Saved") / "RenderForensics";
    std::filesystem::create_directories(dir, ec);
    const std::filesystem::path path = dir / "WHITE_SCREEN_REPORT.txt";
    std::ofstream file(path);
    if (!file) return;

    file << "=== WHITE SCREEN FORENSIC REPORT ===\n";
    file << "Frame: " << report.frameIndex << "\n";
    file << "First failing pass: " << PassName(report.firstAnomalyPass) << "\n";
    file << "Reason: " << report.firstAnomalyReason << "\n";
    file << "Source: " << report.rootCauseFile << ":" << report.rootCauseLine << "\n";
    file << "Explanation: " << report.rootCauseExplanation << "\n";
    file << "Minimal fix: " << report.minimalFix << "\n";
    file << "Conclusion: " << report.conclusion.executiveSummary << "\n\n";

    file << "Pass execution order:\n";
    for (const auto& exec : report.passes) {
        file << "  " << exec.globalOrder << ". " << exec.passName
             << " exec#" << exec.executionNumber
             << " [" << HealthIndicator(exec.health) << "]";
        if (exec.outputStats.valid) {
            file << " white%=" << exec.outputStats.whitePixelPercent
                 << " max=" << FmtRgb(exec.outputStats.maxR, exec.outputStats.maxG, exec.outputStats.maxB)
                 << " center=" << FmtRgb(exec.outputStats.corners.center.r, exec.outputStats.corners.center.g, exec.outputStats.corners.center.b);
        }
        file << "\n";
    }
}

#endif
} // namespace we::runtime::renderer
