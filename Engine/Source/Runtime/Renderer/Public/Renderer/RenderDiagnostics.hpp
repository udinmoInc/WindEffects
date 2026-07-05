#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/SceneRenderer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <volk.h>

namespace we::runtime::renderer {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct DiagnosticMessage {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string subsystem;
    std::string message;
    std::string remediation;
};

enum class PassExecutionStatus {
    Pending,
    Executed,
    Skipped,
    Failed
};

class RenderDiagnostics {
public:
    RENDERER_API static RenderDiagnostics& Get();

    RENDERER_API void ResetFrame();
    RENDERER_API void ValidateStartup(
        const SceneRenderer* sceneRenderer,
        VkRenderPass offscreenRenderPass,
        VkRenderPass swapchainRenderPass);

    RENDERER_API void ValidateEnvironmentUniform(const SceneEnvironmentUniform& env);
    RENDERER_API void ValidatePipeline(const char* passName, VkPipeline pipeline);
    RENDERER_API void ValidateDescriptorSet(const char* passName, uint32_t setIndex, VkDescriptorSet set);
    RENDERER_API void ValidateShaderBytecode(const char* shaderName, bool vertexLoaded, bool pixelLoaded);
    RENDERER_API void ValidateAtmosphereLUTs(bool generated, uint32_t transmittanceW, uint32_t skyViewW);
    RENDERER_API void ValidateAtmosphereLUTResource(
        const char* lutName,
        VkImage image,
        VkImageView view,
        VkSampler sampler,
        uint32_t width,
        uint32_t height,
        VkFormat format);
    RENDERER_API void ValidateFramebuffer(VkFramebuffer fb, uint32_t width, uint32_t height);
    RENDERER_API void RecordPassExecuted(const char* passName, double cpuMs);
    RENDERER_API void RecordPassStatus(const char* passName, PassExecutionStatus status, const char* detail = nullptr);

    RENDERER_API bool HasErrors() const { return m_HasErrors; }
    RENDERER_API bool HasCritical() const { return m_HasCritical; }
    RENDERER_API const std::vector<DiagnosticMessage>& GetMessages() const { return m_Messages; }
    RENDERER_API PassExecutionStatus GetPassStatus(const char* passName) const;
    RENDERER_API std::string GetSummary() const;

    RENDERER_API void Emit(
        DiagnosticSeverity severity,
        const std::string& subsystem,
        const std::string& message,
        const std::string& remediation = {});

private:
    RenderDiagnostics() = default;

    std::vector<DiagnosticMessage> m_Messages;
    bool m_HasErrors = false;
    bool m_HasCritical = false;
    std::vector<std::pair<std::string, double>> m_PassTimings;
    std::unordered_map<std::string, PassExecutionStatus> m_PassStatuses;
    std::unordered_map<std::string, std::string> m_PassStatusDetails;
};

class GpuDebugScope {
public:
    RENDERER_API GpuDebugScope(VkCommandBuffer cmd, const char* label, float r = 0.2f, float g = 0.6f, float b = 1.0f);
    RENDERER_API ~GpuDebugScope();

    GpuDebugScope(const GpuDebugScope&) = delete;
    GpuDebugScope& operator=(const GpuDebugScope&) = delete;

private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    bool m_Active = false;
};

RENDERER_API void InsertGpuDebugLabel(VkCommandBuffer cmd, const char* label, float r = 0.2f, float g = 0.6f, float b = 1.0f);

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
