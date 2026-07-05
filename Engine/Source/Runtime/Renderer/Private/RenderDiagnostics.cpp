#include "Renderer/RenderDiagnostics.hpp"
#include "Core/DiagnosticMacros.hpp"
#include "Core/LogCategory.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace we::runtime::renderer {

RenderDiagnostics& RenderDiagnostics::Get() {
    static RenderDiagnostics instance;
    return instance;
}

void RenderDiagnostics::ResetFrame() {
    m_PassTimings.clear();
    m_PassStatuses.clear();
    m_PassStatusDetails.clear();
}

void RenderDiagnostics::Emit(DiagnosticSeverity severity, const std::string& subsystem, const std::string& message, const std::string& remediation) {
    m_Messages.push_back({ severity, subsystem, message, remediation });
    if (severity == DiagnosticSeverity::Error || severity == DiagnosticSeverity::Critical) {
        m_HasErrors = true;
    }
    if (severity == DiagnosticSeverity::Critical) {
        m_HasCritical = true;
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

    m_Messages.clear();
    m_HasErrors = false;
    m_HasCritical = false;

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
    if (env.atmosphereHeight <= 0.0f || env.planetRadius <= 0.0f) {
        Emit(DiagnosticSeverity::Error, LogCategory::Environment.data(), "Invalid atmosphere/planet radii.", "Use positive atmosphere height and planet radius.");
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
    m_PassStatuses[passName] = status;
    if (detail && detail[0] != '\0') {
        m_PassStatusDetails[passName] = detail;
    } else {
        m_PassStatusDetails.erase(passName);
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
    const auto it = m_PassStatuses.find(passName);
    return it != m_PassStatuses.end() ? it->second : PassExecutionStatus::Pending;
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
    m_PassTimings.emplace_back(passName, cpuMs);
}

std::string RenderDiagnostics::GetSummary() const {
    std::ostringstream ss;
    ss << "Diagnostics: " << m_Messages.size() << " messages";
    if (m_HasCritical) ss << " [CRITICAL]";
    else if (m_HasErrors) ss << " [ERRORS]";
    for (const auto& [pass, ms] : m_PassTimings) {
        ss << " | " << pass << '=' << ms << "ms";
    }
    static const char* kTrackedPasses[] = {
        "AtmosphereLUT", "SkyAtmosphere", "VolumetricClouds", "FogComposite", "PostExposure", "ToneMapping"
    };
    for (const char* pass : kTrackedPasses) {
        const auto it = m_PassStatuses.find(pass);
        if (it != m_PassStatuses.end()) {
            ss << " | " << pass << ':' << PassStatusLabel(it->second);
            const auto detailIt = m_PassStatusDetails.find(pass);
            if (detailIt != m_PassStatusDetails.end() && !detailIt->second.empty()) {
                ss << '(' << detailIt->second << ')';
            }
        }
    }
    return ss.str();
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
