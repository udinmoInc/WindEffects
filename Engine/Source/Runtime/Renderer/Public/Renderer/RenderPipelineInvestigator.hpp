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
#include <array>
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
    /// -1 = use individual disable flags; 0..7 = cumulative sky isolation steps (see SkyIsolationStepName).
    int skyIsolationStep = -1;
    /// True when -PipelineSkyIsolationStep= was present on the command line (even if value is -1).
    bool skyIsolationStepFromCommandLine = false;
    /// -1 = use individual disable flags; 0..5 = cumulative effect steps (see ParseCommandLine).
    int effectStep = -1;
    bool forceConstantBlueSky = false;
    bool forceExposureOne = false;
    float fixedExposureEV = 0.0f;
    float fixedExposureMultiplier = 1.0f;
    bool disableAutoExposure = false;
    bool bypassToneMapping = false;
    bool disableToneMapping = false;
    bool disableSky = false;
    bool disableFog = false;
    bool disableClouds = false;
    bool disableAerialPerspective = false;
    bool disableGeometry = false;
    bool disableBloom = false;
    bool disableSunDisk = false;
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

/// Linear HDR/LDR sample at a fixed world-elevation probe (zenith / mid / horizon).
struct SkyElevationProbe {
    const char* label = "";
    float elevationDeg = 0.0f;
    int pixelX = -1;
    int pixelY = -1;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    bool onScreen = false;
    bool valid = false;
};

struct SkyProbeFrameReport {
    SkyElevationProbe hdrProbes[3]{};
    SkyElevationProbe ldrProbes[3]{};
    bool hasHdrProbes = false;
    bool hasLdrProbes = false;
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
    SkyProbeFrameReport skyProbes{};
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
    RENDERER_API static void ApplyConfigOverrides(RenderPipelineInvestigatorSettings& settings);
    /// Restores the last active sky isolation step from Saved/Config/pipeline_sky_isolation.ini.
    RENDERER_API static void ApplyPersistedOverrides(RenderPipelineInvestigatorSettings& settings);
    RENDERER_API static const char* PassName(RenderPassId pass);
    RENDERER_API static const char* SkyIsolationStepName(int step);
    RENDERER_API void BeginFrame(uint64_t frameIndex);
    RENDERER_API void ApplyEnvironmentOverrides(SceneEnvironmentUniform& uniform) const;
    RENDERER_API bool ShouldRunPass(RenderPassId pass) const;
    RENDERER_API bool ShouldRunPostProcess() const;
    RENDERER_API bool ShouldApplyBloom() const;
    RENDERER_API bool ShouldApplyTonemapping() const;
    RENDERER_API bool ShouldApplyAutoExposure() const;
    RENDERER_API bool ShouldRenderSunDisk() const;
    RENDERER_API void RecordCameraAndEnvironment(const glm::vec3& cameraPos, const SceneEnvironmentUniform& env, float gpuAvgLuminance);
    RENDERER_API void RecordCameraMatrices(const glm::mat4& view, const glm::mat4& proj);
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
    struct ResolvedStageGates;
    ResolvedStageGates ResolveStageGates() const;
    HdrBufferStats AnalyzePixels(const std::vector<float>& rgba, float maxComponent) const;
    bool ValidateStats(const HdrBufferStats& stats, bool isLdr, std::string& outReason) const;
    void WriteReportFile(const PipelineInvestigationReport& report) const;
    void WriteSkyProbeReport(const PipelineInvestigationReport& report) const;
    void LogSkyProbes(const SkyProbeFrameReport& probes) const;
    void LogFrameReport(const PipelineInvestigationReport& report) const;
    HdrBufferStats ReadbackStaging(const VulkanContext& context, const PendingCheckpoint& cp) const;
    std::vector<float> ReadbackStagingPixels(const VulkanContext& context, const PendingCheckpoint& cp, uint32_t& outWidth, uint32_t& outHeight) const;
    std::array<SkyElevationProbe, 3> SampleSkyElevationProbes(
        const std::vector<float>& rgba,
        uint32_t width,
        uint32_t height,
        bool isLdr) const;
    void DestroyPending(const VulkanContext& context);
    void PersistSettings(const RenderPipelineInvestigatorSettings& settings) const;

    RenderPipelineInvestigatorSettings m_Settings{};
    PipelineInvestigationReport m_LastReport{};
    FrameCameraLog m_CameraLog{};
    FrameExecutionAudit m_Audit{};
    glm::mat4 m_CameraView{1.0f};
    glm::mat4 m_CameraProj{1.0f};
    bool m_HasCameraMatrices = false;
    std::vector<PendingCheckpoint> m_Pending{};
    int m_WarmupRemaining = 0;
    bool m_ShouldHalt = false;
};

#endif
} // namespace we::runtime::renderer
