#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Renderer/RenderPipelineInvestigator.hpp"
#include "Renderer/RenderDebugTypes.hpp"
#include "Renderer/RenderGpuInvestigator.hpp"
#include "Renderer/FrameStats.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include "Renderer/Renderer.hpp"
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

struct RenderDebuggerMatrixInfo {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::mat4 inverseView{1.0f};
    glm::mat4 inverseProjection{1.0f};
    bool viewValid = true;
    bool projectionValid = true;
    bool inverseViewValid = true;
    bool inverseProjectionValid = true;
    std::vector<std::string> validationErrors;
};

struct RenderDebuggerCameraInfo {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float nearPlane = 0.1f;
    float farPlane = 100000.0f;
    float fovDegrees = 45.0f;
    RenderDebuggerMatrixInfo matrices{};
};

struct RenderDebuggerEnvironmentInfo {
    glm::vec3 sunDirection{0.0f, 1.0f, 0.0f};
    float sunIntensity = 0.0f;
    float sunAngularRadius = 0.004675f;
    float skyIntensity = 1.0f;
    float groundAlbedo = 0.3f;
    float planetRadius = 6360.0f;
    float atmosphereRadius = 6420.0f;
    float rayleighScaleHeight = 8.0f;
    float mieScaleHeight = 1.2f;
    float ozoneDensity = 0.0f;
};

struct RenderDebuggerExposureInfo {
    bool autoExposure = true;
    float manualExposureEV = 0.0f;
    float ev100 = 0.0f;
    float exposureMultiplier = 1.0f;
    float averageLuminance = 0.0f;
    float histogramMin = 0.0f;
    float histogramMax = 0.0f;
    float histogramAverage = 0.0f;
};

struct RenderDebuggerShaderCenterPixel {
    glm::vec3 viewDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 worldPosition{0.0f};
    glm::vec2 skyViewUV{0.0f};
    glm::vec2 transmittanceUV{0.0f};
    glm::vec3 rayleighRGB{0.0f};
    glm::vec3 mieRGB{0.0f};
    glm::vec3 multiScatterRGB{0.0f};
    glm::vec3 sunDiskRGB{0.0f};
    glm::vec3 finalHdrRGB{0.0f};
    bool valid = false;
};

struct RenderDebuggerPassNode {
    ForensicPassId passId = ForensicPassId::Clear;
    std::string passName;
    bool executed = false;
    double cpuMs = 0.0;
    double gpuMs = 0.0;
    std::string renderTarget;
    std::string textureFormat;
    int inputCount = 0;
    int outputCount = 1;
    ForensicHealth health = ForensicHealth::Pass;
    std::vector<std::string> validationErrors;
};

struct RenderDebuggerResourceInfo {
    std::string name;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string format;
    uint64_t memoryBytes = 0;
    float minR = 0.0f, minG = 0.0f, minB = 0.0f;
    float maxR = 0.0f, maxG = 0.0f, maxB = 0.0f;
    float avgR = 0.0f, avgG = 0.0f, avgB = 0.0f;
    bool valid = false;
};

struct RenderDebuggerCaptureDiff {
    std::string resourceName;
    bool changed = false;
    float maxDeltaR = 0.0f;
    float maxDeltaG = 0.0f;
    float maxDeltaB = 0.0f;
};

struct RenderDebuggerFrameSnapshot {
    uint64_t frameNumber = 0;
    float fps = 0.0f;
    double frameTimeMs = 0.0;
    double gpuTimeMs = 0.0;
    double cpuTimeMs = 0.0;

    RenderDebuggerCameraInfo camera{};
    RenderDebuggerEnvironmentInfo environment{};
    RenderDebuggerExposureInfo exposure{};
    AtmosphereFrameProbe atmosphere{};
    std::vector<RenderDebuggerLUTStats> luts;
    RenderDebuggerShaderCenterPixel centerPixel{};
    std::vector<RenderDebuggerPassNode> renderGraph;
    std::vector<RenderDebuggerResourceInfo> resources;
    std::vector<std::string> validationWarnings;
    RenderDebuggerFirstFailure firstFailure{};

    bool binaryIsolationActive = false;
    std::array<bool, static_cast<size_t>(ForensicPassId::Count)> passEnabled{};
    int captureCount = 0;
    std::vector<RenderDebuggerCaptureDiff> captureDiffs;
    std::string captureStatus;

    GpuInvestigationReport gpuInvestigation;
};

class RenderDebugger {
public:
    RENDERER_API static RenderDebugger& Get();

    RENDERER_API void SetBinaryIsolationEnabled(bool enabled);
    RENDERER_API bool IsBinaryIsolationActive() const { return m_BinaryIsolationEnabled; }

    RENDERER_API void SetPassEnabled(ForensicPassId pass, bool enabled);
    RENDERER_API bool IsPassEnabled(ForensicPassId pass) const;
    RENDERER_API void ResetPassGatesToDefault();

    RENDERER_API bool ShouldRunForensicPass(ForensicPassId pass) const;
    RENDERER_API bool ShouldRunRenderPass(RenderPassId pass) const;

    RENDERER_API bool WantsGpuReadback() const { return m_WantsGpuReadback; }
    RENDERER_API void SetWantsGpuReadback(bool enabled) { m_WantsGpuReadback = enabled; }

    RENDERER_API void RequestCapture();
    RENDERER_API bool ConsumeCaptureRequest();

    RENDERER_API RenderDebuggerFrameSnapshot BuildSnapshot(
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
        float probeDepth = 0.0f,
        float maxHdrComponent = 100.0f);

    RENDERER_API const RenderDebuggerFrameSnapshot& GetLastSnapshot() const { return m_LastSnapshot; }

    RENDERER_API static RenderDebuggerMatrixInfo ValidateMatrices(
        const glm::mat4& view,
        const glm::mat4& projection);

    RENDERER_API static RenderDebuggerLUTStats AnalyzeLUTRGBA(
        const char* name,
        const std::vector<float>& rgba,
        uint32_t width,
        uint32_t height);

private:
    RenderDebugger();

    void RecordCapture(const RenderDebuggerFrameSnapshot& snapshot);
    bool MapForensicPassToRenderPass(ForensicPassId forensic, RenderPassId render) const;

    bool m_BinaryIsolationEnabled = false;
    bool m_WantsGpuReadback = false;
    bool m_CaptureRequested = false;
    std::array<bool, static_cast<size_t>(ForensicPassId::Count)> m_PassEnabled{};
    RenderDebuggerFrameSnapshot m_LastSnapshot{};
    RenderDebuggerFrameSnapshot m_PreviousCapture{};
    bool m_HasPreviousCapture = false;
};

#endif
} // namespace we::runtime::renderer
