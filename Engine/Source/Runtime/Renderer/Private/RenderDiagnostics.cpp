#include "Renderer/RenderDiagnostics.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_map>

namespace we::runtime::renderer {

struct RenderDiagnostics::Impl {
    std::vector<DiagnosticMessage> messages;
    bool hasErrors = false;
    bool hasCritical = false;
    std::vector<std::pair<std::string, double>> passTimings;
    std::unordered_map<std::string, PassExecutionStatus> passStatuses;
    std::unordered_map<std::string, std::string> passStatusDetails;
};

RenderDiagnostics::RenderDiagnostics() : m_Impl(std::make_unique<Impl>()) {}

RenderDiagnostics::~RenderDiagnostics() = default;

RenderDiagnostics& RenderDiagnostics::Get() {
    static RenderDiagnostics instance;
    return instance;
}

void RenderDiagnostics::ResetFrame() {
    m_Impl->passTimings.clear();
    m_Impl->passStatuses.clear();
    m_Impl->passStatusDetails.clear();
}

void RenderDiagnostics::Emit(DiagnosticSeverity severity, const std::string& subsystem, const std::string& message, const std::string& remediation) {
    m_Impl->messages.push_back({ severity, subsystem, message, remediation });
    if (severity == DiagnosticSeverity::Error || severity == DiagnosticSeverity::Critical) {
        m_Impl->hasErrors = true;
    }
    if (severity == DiagnosticSeverity::Critical) {
        m_Impl->hasCritical = true;
    }

    std::string fullMessage = message;
    if (!remediation.empty()) {
        fullMessage += " | Remediation: " + remediation;
    }

    switch (severity) {
        case DiagnosticSeverity::Info:
            WE_LOG_INFO(subsystem.c_str(), fullMessage);
            break;
        case DiagnosticSeverity::Warning:
            WE_LOG_WARN(subsystem.c_str(), fullMessage);
            break;
        case DiagnosticSeverity::Error:
            WE_LOG_ERROR(subsystem.c_str(), fullMessage);
            break;
        case DiagnosticSeverity::Critical:
            WE_LOG_CRITICAL(subsystem.c_str(), fullMessage);
            break;
    }
}

void RenderDiagnostics::ValidateStartup(
    const SceneRenderer* sceneRenderer,
    VkRenderPass offscreenRenderPass,
    VkRenderPass swapchainRenderPass) {

    m_Impl->messages.clear();
    m_Impl->hasErrors = false;
    m_Impl->hasCritical = false;

    if (!sceneRenderer) {
        Emit(DiagnosticSeverity::Critical, LogCategory::Renderer.data(), "SceneRenderer is null at startup.", "Ensure SceneRenderer is constructed before environment binding.");
        return;
    }

    if (offscreenRenderPass == VK_NULL_HANDLE) {
        Emit(DiagnosticSeverity::Critical, LogCategory::Renderer.data(), "Offscreen render pass is null.", "Verify Renderer::CreateRenderPasses succeeded.");
    }
    if (swapchainRenderPass == VK_NULL_HANDLE) {
        Emit(DiagnosticSeverity::Critical, LogCategory::Renderer.data(), "Swapchain render pass is null.", "Verify swapchain render pass creation.");
    }

    if (sceneRenderer->GetEnvironmentBuffer() == VK_NULL_HANDLE) {
        Emit(DiagnosticSeverity::Critical, LogCategory::Environment.data(), "Environment uniform buffer was not created.", "Check SceneRenderer constructor buffer allocation.");
    }

    WE_LOG_INFO(LogCategory::Startup.data(), "Render diagnostics startup validation completed.");
}

void RenderDiagnostics::ValidateEnvironmentUniform(const SceneEnvironmentUniform& env) {
    if (env.sunIntensity < 0.0f || !std::isfinite(env.sunIntensity)) {
        Emit(DiagnosticSeverity::Error, LogCategory::Environment.data(), "Invalid sun intensity.", "Set Directional Sun intensity >= 0.");
    }
    if (env.sunColor.x < 0.05f) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            "Sun color red channel is near zero in the environment uniform.",
            "Verify TemperatureKelvinToRgb uses 255 (not 1) for the red/blue plateaus in EnvironmentLighting.cpp.");
    }
    const float rayleighGR = env.atmosphereRayleigh.y / std::max(env.atmosphereRayleigh.x, 1e-6f);
    if (rayleighGR > 20.0f || rayleighGR < 1.5f) {
        Emit(
            DiagnosticSeverity::Warning,
            LogCategory::Environment.data(),
            "Rayleigh scattering G/R ratio looks unusual: " + std::to_string(rayleighGR) + ".",
            "Expected ~2.3 for Earth-like coefficients from GetRayleighColor().");
    }
    if (env.atmosphereHeight <= 0.0f || env.planetRadius <= 0.0f) {
        Emit(DiagnosticSeverity::Error, LogCategory::Environment.data(), "Invalid atmosphere/planet radii.", "Use positive atmosphere height and planet radius.");
    }
    if (!std::isfinite(env.exposureEV) || env.exposureEV < -2.0f || env.exposureEV > 14.0f) {
        Emit(DiagnosticSeverity::Error, LogCategory::Environment.data(),
            "exposureEV out of expected range: " + std::to_string(env.exposureEV),
            "Clamp exposure in EnvironmentExposureController.");
    }
    if (!std::isfinite(env.hdrSkyLuminance) || env.hdrSkyLuminance < 0.0f || env.hdrSkyLuminance > 1000.0f) {
        Emit(DiagnosticSeverity::Error, LogCategory::Environment.data(),
            "hdrSkyLuminance out of expected range: " + std::to_string(env.hdrSkyLuminance),
            "Verify EnvironmentManager::ComputeHdrSkyLuminance.");
    }
    if (env.enableAutoExposure > 0.5f && env.hdrSkyLuminance < 1e-4f) {
        Emit(DiagnosticSeverity::Warning, LogCategory::Environment.data(),
            "Auto exposure enabled with hdrSkyLuminance near zero.",
            "Disable auto exposure or set a positive hdrSkyLuminance floor.");
    }
    const float sunDirLen = std::sqrt(env.sunDirection.x * env.sunDirection.x + env.sunDirection.y * env.sunDirection.y + env.sunDirection.z * env.sunDirection.z);
    if (sunDirLen < 0.001f) {
        Emit(DiagnosticSeverity::Warning, LogCategory::Environment.data(), "Sun direction is near zero.", "Normalize Directional Sun rotation.");
    }
}

void RenderDiagnostics::ValidatePipeline(const char* passName, VkPipeline pipeline) {
    if (pipeline == VK_NULL_HANDLE) {
        Emit(
            DiagnosticSeverity::Critical,
            LogCategory::Pipeline.data(),
            std::string("Pipeline for pass '") + passName + "' is null.",
            "Compile shader bytecodes and verify descriptor set layouts match HLSL register spaces.");
    }
}

void RenderDiagnostics::ValidateDescriptorSet(const char* passName, uint32_t setIndex, VkDescriptorSet set) {
    if (set == VK_NULL_HANDLE) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Resource.data(),
            std::string("Descriptor set ") + std::to_string(setIndex) + " for pass '" + passName + "' is null.",
            "Ensure descriptor pool allocation succeeded and bindings were updated.");
    }
}

void RenderDiagnostics::ValidateShaderBytecode(const char* shaderName, bool vertexLoaded, bool pixelLoaded) {
    if (!vertexLoaded || !pixelLoaded) {
        Emit(
            DiagnosticSeverity::Critical,
            LogCategory::Shader.data(),
            std::string("Missing shader bytecode for '") + shaderName + "' (VS=" + (vertexLoaded ? "ok" : "missing") + ", PS=" + (pixelLoaded ? "ok" : "missing") + ").",
            "Run Engine/Shaders/CompileShaders.ps1 and verify Engine/Shaders/Bytecodes is on the shader search path.");
    }
}

void RenderDiagnostics::ValidateAtmosphereLUTs(bool generated, uint32_t transmittanceW, uint32_t skyViewW) {
    if (!generated) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            "Atmosphere LUTs were not generated.",
            "Verify AtmosphereLUTGenerator::EnsureGenerated is called before the sky pass.");
        return;
    }
    if (transmittanceW == 0 || skyViewW == 0) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            "Atmosphere LUT dimensions are invalid.",
            "Regenerate LUT textures with non-zero dimensions.");
    }
}

void RenderDiagnostics::ValidateAtmosphereLUTResource(
    const char* lutName,
    VkImage image,
    VkImageView view,
    VkSampler sampler,
    uint32_t width,
    uint32_t height,
    VkFormat format) {

    if (image == VK_NULL_HANDLE) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            std::string(lutName) + " LUT image handle is null.",
            "Recreate AtmosphereLUTGenerator GPU images.");
        return;
    }
    if (view == VK_NULL_HANDLE) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Resource.data(),
            std::string(lutName) + " LUT image view is null.",
            "Verify CreateImageView succeeded for atmosphere LUT textures.");
        return;
    }
    if (sampler == VK_NULL_HANDLE) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Resource.data(),
            std::string(lutName) + " LUT sampler is null.",
            "Verify atmosphere LUT sampler creation.");
        return;
    }
    if (width == 0 || height == 0) {
        Emit(
            DiagnosticSeverity::Error,
            LogCategory::Environment.data(),
            std::string(lutName) + " LUT has zero dimensions.",
            "Use non-zero LUT dimensions in AtmosphereLUTGenerator.");
        return;
    }
    if (format != VK_FORMAT_R32G32B32A32_SFLOAT) {
        Emit(
            DiagnosticSeverity::Warning,
            LogCategory::Environment.data(),
            std::string(lutName) + " LUT format is unexpected.",
            "Atmosphere LUTs should use VK_FORMAT_R32G32B32A32_SFLOAT.");
    }
}

void RenderDiagnostics::RecordPassStatus(const char* passName, PassExecutionStatus status, const char* detail) {
    if (!passName) return;
    m_Impl->passStatuses[passName] = status;
    if (detail && detail[0] != '\0') {
        m_Impl->passStatusDetails[passName] = detail;
    } else {
        m_Impl->passStatusDetails.erase(passName);
    }

    if (status == PassExecutionStatus::Failed) {
        const std::string message = std::string("Render pass '") + passName + "' failed"
            + (detail ? std::string(": ") + detail : std::string("."));
        Emit(DiagnosticSeverity::Error, LogCategory::Renderer.data(), message);
    } else if (status == PassExecutionStatus::Skipped && detail) {
        WE_LOG_WARN(LogCategory::Renderer.data(), std::string("Render pass '") + passName + "' skipped: " + detail);
    }
}

PassExecutionStatus RenderDiagnostics::GetPassStatus(const char* passName) const {
    if (!passName) return PassExecutionStatus::Pending;
    const auto it = m_Impl->passStatuses.find(passName);
    return it != m_Impl->passStatuses.end() ? it->second : PassExecutionStatus::Pending;
}

namespace {
const char* PassStatusLabel(PassExecutionStatus status) {
    switch (status) {
        case PassExecutionStatus::Executed: return "executed";
        case PassExecutionStatus::Skipped: return "skipped";
        case PassExecutionStatus::Failed: return "failed";
        default: return "pending";
    }
}
} // namespace

void RenderDiagnostics::ValidateFramebuffer(VkFramebuffer fb, uint32_t width, uint32_t height) {
    if (fb == VK_NULL_HANDLE) {
        Emit(DiagnosticSeverity::Critical, LogCategory::Resource.data(), "Framebuffer handle is null.", "Recreate offscreen framebuffer after resize.");
    }
    if (width == 0 || height == 0) {
        Emit(DiagnosticSeverity::Error, LogCategory::Resource.data(), "Framebuffer has zero dimension.", "Ensure viewport size is valid before rendering.");
    }
}

void RenderDiagnostics::RecordPassExecuted(const char* passName, double cpuMs) {
    m_Impl->passTimings.emplace_back(passName, cpuMs);
}

std::string RenderDiagnostics::GetSummary() const {
    std::ostringstream ss;
    ss << "Diagnostics: " << m_Impl->messages.size() << " messages";
    if (m_Impl->hasCritical) ss << " [CRITICAL]";
    else if (m_Impl->hasErrors) ss << " [ERRORS]";
    for (const auto& [pass, ms] : m_Impl->passTimings) {
        ss << " | " << pass << '=' << ms << "ms";
    }
    static const char* kTrackedPasses[] = {
        "AtmosphereLUT", "SkyAtmosphere", "VolumetricClouds", "FogComposite", "PostExposure", "ToneMapping"
    };
    for (const char* pass : kTrackedPasses) {
        const auto it = m_Impl->passStatuses.find(pass);
        if (it != m_Impl->passStatuses.end()) {
            ss << " | " << pass << ':' << PassStatusLabel(it->second);
            const auto detailIt = m_Impl->passStatusDetails.find(pass);
            if (detailIt != m_Impl->passStatusDetails.end() && !detailIt->second.empty()) {
                ss << '(' << detailIt->second << ')';
            }
        }
    }
    return ss.str();
}

bool RenderDiagnostics::HasErrors() const {
    return m_Impl->hasErrors;
}

bool RenderDiagnostics::HasCritical() const {
    return m_Impl->hasCritical;
}

const std::vector<DiagnosticMessage>& RenderDiagnostics::GetMessages() const {
    return m_Impl->messages;
}

#if WE_HAS_VULKAN

void InsertGpuDebugLabel(VkCommandBuffer cmd, const char* label, float r, float g, float b) {
    if (cmd == VK_NULL_HANDLE || label == nullptr) return;
    if (vkCmdInsertDebugUtilsLabelEXT == nullptr) return;

    VkDebugUtilsLabelEXT debugLabel{};
    debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    debugLabel.pLabelName = label;
    debugLabel.color[0] = r;
    debugLabel.color[1] = g;
    debugLabel.color[2] = b;
    debugLabel.color[3] = 1.0f;
    vkCmdInsertDebugUtilsLabelEXT(cmd, &debugLabel);
}

GpuDebugScope::GpuDebugScope(VkCommandBuffer cmd, const char* label, float r, float g, float b)
    : m_CommandBuffer(cmd) {
    if (cmd == VK_NULL_HANDLE || vkCmdBeginDebugUtilsLabelEXT == nullptr) return;
    VkDebugUtilsLabelEXT debugLabel{};
    debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    debugLabel.pLabelName = label;
    debugLabel.color[0] = r;
    debugLabel.color[1] = g;
    debugLabel.color[2] = b;
    debugLabel.color[3] = 1.0f;
    vkCmdBeginDebugUtilsLabelEXT(cmd, &debugLabel);
    m_Active = true;
}

GpuDebugScope::~GpuDebugScope() {
    if (m_Active && vkCmdEndDebugUtilsLabelEXT != nullptr) {
        vkCmdEndDebugUtilsLabelEXT(m_CommandBuffer);
    }
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
