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

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

enum class ForensicPassId : int {
    Clear = 0,
    Shadow,
    AtmosphereLUT,
    Geometry,
    Lighting,
    SkyAtmosphere,
    VolumetricClouds,
    Fog,
    AerialPerspective,
    Grid,
    Gizmos,
    SceneComposite,
    LuminanceReduce,
    Bloom,
    Exposure,
    ToneMapping,
    UI,
    Present,
    Count
};

enum class ForensicHealth {
    Pass = 0,
    Warning,
    Error
};

struct ForensicPixelRGBA {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct ForensicCornerSamples {
    ForensicPixelRGBA center{};
    ForensicPixelRGBA topLeft{};
    ForensicPixelRGBA topRight{};
    ForensicPixelRGBA bottomLeft{};
    ForensicPixelRGBA bottomRight{};
};

struct ForensicTargetStats {
    float minR = 0.0f, minG = 0.0f, minB = 0.0f;
    float maxR = 0.0f, maxG = 0.0f, maxB = 0.0f;
    float avgR = 0.0f, avgG = 0.0f, avgB = 0.0f;
    float avgLuminance = 0.0f;
    uint32_t nanCount = 0;
    uint32_t infCount = 0;
    uint32_t over10Count = 0;
    uint32_t over100Count = 0;
    float whitePixelPercent = 0.0f;
    float blackPixelPercent = 0.0f;
    bool allPixelsSameColor = false;
    uint32_t samples = 0;
    bool valid = false;
    ForensicCornerSamples corners{};
};

struct ForensicPassMetadata {
    std::string inputTarget = "n/a";
    std::string outputTarget = "OffscreenColor";
    uint32_t width = 0;
    uint32_t height = 0;
    std::string format = "R16G16B16A16_SFLOAT";
    std::string inputLayout = "COLOR_ATTACHMENT_OPTIMAL";
    std::string outputLayout = "COLOR_ATTACHMENT_OPTIMAL";
    std::string clearColor = "n/a";
    std::string loadOp = "LOAD";
    std::string storeOp = "STORE";
    std::string blendState = "n/a";
    std::string depthState = "n/a";
    std::string viewport = "full";
    std::string scissor = "full";
    std::string pipelineName;
    std::string vertexShader;
    std::string pixelShader;
    std::string computeShader;
    std::string descriptorSets;
    std::string pushConstants;
    std::string boundTextures;
    std::string sourceFile;
    int sourceLine = 0;
};

struct ForensicPassDelta {
    bool valid = false;
    float avgRBefore = 0.0f, avgGBefore = 0.0f, avgBBefore = 0.0f;
    float avgRAfter = 0.0f, avgGAfter = 0.0f, avgBAfter = 0.0f;
    float avgRDiff = 0.0f, avgGDiff = 0.0f, avgBDiff = 0.0f;
    float whitePercentBefore = 0.0f;
    float whitePercentAfter = 0.0f;
    ForensicPixelRGBA centerBefore{};
    ForensicPixelRGBA centerAfter{};
    bool rgbJumpExceeds2x = false;
};

struct ForensicExposureDetails {
    int executionNumber = 0;
    float avgSceneLuminance = 0.0f;
    float ev100 = 0.0f;
    float exposureMultiplier = 0.0f;
    ForensicTargetStats inputHdr{};
    ForensicTargetStats outputHdr{};
    std::vector<std::string> callStack;
};

struct ForensicPassExecution {
    int globalOrder = 0;
    int executionNumber = 1;
    ForensicPassId passId = ForensicPassId::Clear;
    std::string passName;
    bool executed = false;
    bool succeeded = true;
    ForensicHealth health = ForensicHealth::Pass;
    double cpuMs = 0.0;
    double gpuMs = 0.0;
    ForensicPassMetadata metadata{};
    ForensicTargetStats inputStats{};
    ForensicTargetStats outputStats{};
    ForensicPassDelta delta{};
    std::vector<std::string> callStack;
    std::vector<std::string> validationErrors;
    bool hasExposureDetails = false;
    ForensicExposureDetails exposureDetails{};
};

// Backward-compatible alias used by the diagnostics panel.
using ForensicPassRecord = ForensicPassExecution;

struct ForensicCameraLog {
    float cameraX = 0.0f, cameraY = 0.0f, cameraZ = 0.0f;
    float cameraDirX = 0.0f, cameraDirY = 0.0f, cameraDirZ = 0.0f;
    float cameraHeight = 0.0f;
    float planetRadius = 0.0f;
    float atmosphereRadius = 0.0f;
    float sunDirX = 0.0f, sunDirY = 0.0f, sunDirZ = 0.0f;
    float sunColorR = 1.0f, sunColorG = 1.0f, sunColorB = 1.0f;
    float sunIntensity = 0.0f;
    float exposureEV = 0.0f;
    float exposureMultiplier = 0.0f;
    float ev100 = 0.0f;
    float avgSceneLuminance = 0.0f;
    ForensicTargetStats hdrBeforeExposure{};
    ForensicTargetStats hdrAfterExposure{};
    ForensicTargetStats hdrBeforeToneMap{};
    ForensicTargetStats finalLdr{};
};

struct ForensicExecutionAudit {
    std::array<int, static_cast<size_t>(ForensicPassId::Count)> passCounts{};
};

struct ForensicInvestigationConclusion {
    ForensicPassId firstInvalidOutputPass = ForensicPassId::Count;
    std::string firstInvalidOutputReason;
    bool framebufferInvalidBeforeExposure = false;
    bool duplicateExposureIsPrimaryCause = false;
    bool duplicateExposureIsInstrumentationArtifact = false;
    std::string duplicateExposureAnalysis;
    std::string rootCauseFile;
    int rootCauseLine = 0;
    std::string rootCauseFunction;
    std::string minimalFix;
    std::string executiveSummary;
};

struct ForensicFrameReport {
    uint64_t frameIndex = 0;
    ForensicHealth frameHealth = ForensicHealth::Pass;
    bool frameFailed = false;
    bool haltedForWhiteScreen = false;
    ForensicPassId firstAnomalyPass = ForensicPassId::Count;
    std::string firstAnomalyReason;
    std::string rootCauseFile;
    int rootCauseLine = 0;
    std::string rootCauseExplanation;
    std::string minimalFix;
    ForensicCameraLog cameraLog{};
    ForensicExecutionAudit audit{};
    ForensicInvestigationConclusion conclusion{};
    std::vector<ForensicPassExecution> passes;
};

struct RenderForensicsSettings {
    bool enabled = false;
    bool enableGpuReadback = false;
    bool logEveryFrame = true;
    bool writeReportFile = true;
    bool haltOnInvalid = true;
    bool haltOnWhiteScreen = true;
    float maxHdrComponent = 100.0f;
    float maxExposure = 32.0f;
    float hdrWhitePixelThreshold = 64.0f;
    float ldrWhitePixelThreshold = 0.99f;
    float blackPixelThreshold = 0.01f;
    float whiteScreenPercent = 95.0f;
    float maxRgbJumpRatio = 2.0f;
    int warmupFrames = 1;
    int sampleStride = 4;
    int maxCallStackFrames = 24;
};

class RenderForensics {
public:
    RENDERER_API static RenderForensics& Get();

    RENDERER_API void Configure(const RenderForensicsSettings& settings);
    RENDERER_API bool IsActive() const { return m_Settings.enabled; }
    RENDERER_API bool IsReadbackEnabled() const { return m_Settings.enabled && m_Settings.enableGpuReadback; }
    RENDERER_API const RenderForensicsSettings& GetSettings() const { return m_Settings; }

    RENDERER_API static const char* PassName(ForensicPassId pass);
    RENDERER_API static bool IsLdrPass(ForensicPassId pass);
    RENDERER_API static RenderForensicsSettings DefaultEditorSettings();

    RENDERER_API void BeginFrame(uint64_t frameIndex);
    RENDERER_API void RecordCameraAndEnvironment(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        const SceneEnvironmentUniform& env,
        float gpuAvgLuminance);

    /// Returns execution index for correlating timing, readback, and exposure details.
    RENDERER_API int BeginPassExecution(
        ForensicPassId pass,
        ForensicPassMetadata metadata,
        const char* sourceFile,
        int sourceLine);

    RENDERER_API void EndPassExecution(int executionIndex, double cpuMs, bool succeeded = true);
    RENDERER_API void RecordExposureDetails(
        int executionIndex,
        float ev100,
        float exposureMultiplier,
        float avgSceneLuminance);

    RENDERER_API void AuditPassBegin(ForensicPassId pass);
    RENDERER_API void RecordPassMetadata(ForensicPassId pass, ForensicPassMetadata metadata);
    RENDERER_API void RecordPassTiming(ForensicPassId pass, double cpuMs, bool succeeded = true);

    RENDERER_API void EnqueueReadback(
        VkCommandBuffer cmd,
        const VulkanContext& context,
        VkImage colorImage,
        uint32_t width,
        uint32_t height,
        ForensicPassId pass,
        VkImageLayout currentLayout,
        int executionIndex = -1);

    RENDERER_API ForensicFrameReport FinalizeFrame(const VulkanContext& context, VkFence fence);
    RENDERER_API bool ShouldHalt() const { return m_ShouldHalt; }
    RENDERER_API const ForensicFrameReport& GetLastReport() const { return m_LastReport; }
    RENDERER_API const ForensicCameraLog& GetCurrentCameraLog() const { return m_CameraLog; }

private:
    struct PendingReadback {
        int executionIndex = -1;
        ForensicPassId pass = ForensicPassId::Clear;
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    RenderForensics() = default;

    ForensicPassExecution& ExecutionAt(int index);
    const ForensicPassExecution& ExecutionAt(int index) const;
    ForensicTargetStats AnalyzePixels(const std::vector<float>& rgba, uint32_t width, uint32_t height, bool isLdr) const;
    ForensicTargetStats ReadbackStaging(const VulkanContext& context, const PendingReadback& pending) const;
    ForensicPassDelta ComputeDelta(const ForensicTargetStats& before, const ForensicTargetStats& after) const;
    ForensicHealth ClassifyPassHealth(const ForensicPassExecution& record, bool isLdr) const;
    void ValidatePassExecution(ForensicPassExecution& record, bool isLdr);
    void DetectAnomalies(ForensicFrameReport& report);
    void BuildInvestigationConclusion(ForensicFrameReport& report);
    void LogPassExecution(const ForensicPassExecution& record) const;
    void LogPassDelta(const ForensicPassExecution& record) const;
    void LogCameraState(const ForensicCameraLog& log) const;
    void LogFrameReport(const ForensicFrameReport& report) const;
    void WriteReportFile(const ForensicFrameReport& report) const;
    void WriteFinalDiagnosticReport(const ForensicFrameReport& report) const;
    void WriteWhiteScreenReport(const ForensicFrameReport& report) const;
    void DestroyPending(const VulkanContext& context);
    std::vector<std::string> CaptureCallStack(int maxFrames) const;

    RenderForensicsSettings m_Settings{};
    ForensicFrameReport m_LastReport{};
    ForensicCameraLog m_CameraLog{};
    ForensicExecutionAudit m_Audit{};
    std::vector<PendingReadback> m_Pending{};
    std::vector<ForensicPassExecution> m_Executions{};
    int m_NextGlobalOrder = 0;
    int m_WarmupRemaining = 0;
    bool m_ShouldHalt = false;
};

#define WE_FORENSIC_PASS_BEGIN(forensics, passId, meta) \
    (forensics).BeginPassExecution((passId), (meta), __FILE__, __LINE__)

#endif
} // namespace we::runtime::renderer
