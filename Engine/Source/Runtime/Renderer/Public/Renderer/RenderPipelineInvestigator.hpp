#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include "Renderer/VulkanContext.hpp"
#include <volk.h>
#endif

#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

enum class RenderPassId : int {
    Clear = 0,
    SkyAtmosphere,
    Geometry,
    Lighting,
    VolumetricClouds,
    Fog,
    AerialPerspective,
    PostProcessing,
    Exposure,
    ToneMapping,
    Present,
    Count
};

struct RenderPipelineInvestigatorSettings {
    bool enabled = false;
    bool enableGpuReadback = false;
    bool haltOnInvalid = true;
    bool logEveryFrame = true;
    bool writeReportFile = true;
    float maxHdrComponent = 100.0f;
    int warmupFrames = 2;
    int sampleStride = 8;
    int onlyPass = -1;
    /// -1 = use individual disable flags; 0..5 = cumulative effect steps (see ParseCommandLine).
    int effectStep = -1;
    bool forceConstantBlueSky = false;
    bool forceExposureOne = false;
    bool disableAutoExposure = false;
    bool bypassToneMapping = false;
    bool disableSky = false;
    bool disableFog = false;
    bool disableClouds = false;
    bool disableAerialPerspective = false;
    bool disableGeometry = false;
    bool disableBloom = false;
    bool disablePostProcessing = false;
    bool disableGrid = false;
};

struct HdrBufferStats {
    float minR = 0.0f, minG = 0.0f, minB = 0.0f;
    float maxR = 0.0f, maxG = 0.0f, maxB = 0.0f;
    float avgLuminance = 0.0f;
    uint32_t nanCount = 0, infCount = 0, negativeCount = 0, overLimitCount = 0;
    uint32_t samples = 0;
    bool valid = false;
};

struct PassCheckpointResult {
    RenderPassId pass = RenderPassId::Clear;
    HdrBufferStats stats{};
    bool failed = false;
    std::string failureReason;
};

struct FrameCameraLog {
    float cameraX = 0, cameraY = 0, cameraZ = 0, cameraHeight = 0;
    float planetRadius = 0, atmosphereRadius = 0;
    float sunDirX = 0, sunDirY = 0, sunDirZ = 0, sunIntensity = 0;
    float exposureEV = 0, hdrSkyLuminance = 0, avgLuminanceGpu = 0;
    HdrBufferStats hdrBeforeExposure{}, hdrAfterExposure{}, hdrBeforeToneMap{}, finalLdr{};
};

struct FrameExecutionAudit {
    int clearPassCount = 0, skyPassCount = 0, geometryPassCount = 0;
    int cloudsPassCount = 0, fogPassCount = 0;
    int exposurePassCount = 0, toneMapPassCount = 0, presentPassCount = 0;
};

struct PipelineInvestigationReport {
    uint64_t frameIndex = 0;
    bool frameFailed = false;
    RenderPassId firstFailingPass = RenderPassId::Count;
    std::string firstFailureReason, minimalFixHint;
    FrameCameraLog cameraLog{};
    FrameExecutionAudit audit{};
    std::vector<PassCheckpointResult> checkpoints;
};

class RenderPipelineInvestigator {
public:
    RENDERER_API static RenderPipelineInvestigator& Get();
    RENDERER_API void Configure(const RenderPipelineInvestigatorSettings& settings);
    RENDERER_API bool IsActive() const { return m_Settings.enabled; }
    RENDERER_API bool IsReadbackEnabled() const { return m_Settings.enabled && m_Settings.enableGpuReadback; }
    RENDERER_API bool ShouldRenderGrid() const;
    RENDERER_API const RenderPipelineInvestigatorSettings& GetSettings() const { return m_Settings; }
    RENDERER_API static RenderPipelineInvestigatorSettings ParseCommandLine(int argc, char* argv[]);
    RENDERER_API static const char* PassName(RenderPassId pass);
    RENDERER_API void BeginFrame(uint64_t frameIndex);
    RENDERER_API void ApplyEnvironmentOverrides(SceneEnvironmentUniform& uniform) const;
    RENDERER_API bool ShouldRunPass(RenderPassId pass) const;
    RENDERER_API void RecordCameraAndEnvironment(const glm::vec3& cameraPos, const SceneEnvironmentUniform& env, float gpuAvgLuminance);
    RENDERER_API void AuditPassBegin(RenderPassId pass);
    RENDERER_API void AuditPassEnd(RenderPassId pass);
    RENDERER_API void EnqueueCheckpoint(VkCommandBuffer cmd, const VulkanContext& context, VkImage colorImage, uint32_t width, uint32_t height, RenderPassId pass);
    RENDERER_API void StoreIntermediateStats(RenderPassId pass, const HdrBufferStats& stats);
    RENDERER_API PipelineInvestigationReport FinalizeFrame(const VulkanContext& context, VkFence fence);
    RENDERER_API bool ShouldHalt() const { return m_ShouldHalt; }
    RENDERER_API const PipelineInvestigationReport& GetLastReport() const { return m_LastReport; }

private:
    struct PendingCheckpoint {
        RenderPassId pass = RenderPassId::Clear;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        uint32_t width = 0, height = 0;
    };

    RenderPipelineInvestigator() = default;
    HdrBufferStats AnalyzePixels(const std::vector<float>& rgba, float maxComponent) const;
    bool ValidateStats(const HdrBufferStats& stats, bool isLdr, std::string& outReason) const;
    void WriteReportFile(const PipelineInvestigationReport& report) const;
    void LogFrameReport(const PipelineInvestigationReport& report) const;
    HdrBufferStats ReadbackStaging(const VulkanContext& context, const PendingCheckpoint& cp) const;
    void DestroyPending(const VulkanContext& context);

    RenderPipelineInvestigatorSettings m_Settings{};
    PipelineInvestigationReport m_LastReport{};
    FrameCameraLog m_CameraLog{};
    FrameExecutionAudit m_Audit{};
    std::vector<PendingCheckpoint> m_Pending{};
    int m_WarmupRemaining = 0;
    bool m_ShouldHalt = false;
};

#endif
} // namespace we::runtime::renderer
