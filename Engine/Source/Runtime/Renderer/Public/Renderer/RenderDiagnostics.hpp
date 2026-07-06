#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/SceneRenderer.hpp"
#include <memory>
#include <string>
#include <vector>
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

    RENDERER_API bool HasErrors() const;
    RENDERER_API bool HasCritical() const;
    RENDERER_API const std::vector<DiagnosticMessage>& GetMessages() const;
    RENDERER_API PassExecutionStatus GetPassStatus(const char* passName) const;
    RENDERER_API std::string GetSummary() const;

    RENDERER_API void Emit(
        DiagnosticSeverity severity,
        const std::string& subsystem,
        const std::string& message,
        const std::string& remediation = {});

private:
    RENDERER_API RenderDiagnostics();
    RENDERER_API ~RenderDiagnostics();

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
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

// Forward declaration — full API in RenderPipelineInvestigator.hpp
class RenderPipelineInvestigator;

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
