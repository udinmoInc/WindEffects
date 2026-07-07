#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Renderer/RenderDebugTypes.hpp"
#include "Renderer/AtmosphereRadianceTracer.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include "Renderer/Renderer.hpp"
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

struct GpuResourceCatalogEntry {
    std::string id;
    std::string displayName;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string format;
    uint64_t memoryBytes = 0;
    std::string producerPass;
    std::vector<std::string> consumerPasses;
    bool exists = false;
};

struct GpuRenderTargetInfo {
    std::string id;
    std::string displayName;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string format;
    uint64_t memoryBytes = 0;
    std::string producerPass;
    std::vector<std::string> consumerPasses;
    bool exists = false;
    ForensicTargetStats stats{};
    std::vector<uint8_t> previewRgba;
    uint32_t previewWidth = 0;
    uint32_t previewHeight = 0;
};

struct GpuPassValidation {
    ForensicPassId passId = ForensicPassId::Clear;
    std::string passName;
    bool executed = false;
    double cpuMs = 0.0;
    double gpuMs = 0.0;
    std::vector<std::string> inputTextures;
    std::vector<std::string> outputTextures;
    ForensicTargetStats outputStats{};
    ForensicPixelRGBA probePixel{};
    bool abnormal = false;
    std::vector<std::string> abnormalities;
    std::string vertexShader;
    std::string pixelShader;
    std::string computeShader;
};

struct GpuPixelPipelineStage {
    ForensicPassId passId = ForensicPassId::Clear;
    std::string passName;
    ForensicPixelRGBA hdrRgb{};
    ForensicPixelRGBA ldrRgb{};
    float depth = 0.0f;
    glm::vec3 worldPosition{0.0f};
    glm::vec3 viewDirection{0.0f};
    glm::vec3 sunDirection{0.0f};
    glm::vec2 skyViewUV{0.0f};
    glm::vec2 transmittanceUV{0.0f};
    glm::vec3 rayleighRGB{0.0f};
    glm::vec3 mieRGB{0.0f};
    glm::vec3 multiScatterRGB{0.0f};
    glm::vec3 sunRGB{0.0f};
    glm::vec3 finalHdrRGB{0.0f};
    glm::vec3 finalLdrRGB{0.0f};
    bool valid = false;
    bool firstInvalidStage = false;
    std::string invalidReason;
};

struct GpuLutInspection {
    std::string name;
    RenderDebuggerLUTStats stats{};
    std::array<uint32_t, 64> luminanceHistogram{};
    std::vector<uint8_t> previewRgba;
    uint32_t previewWidth = 0;
    uint32_t previewHeight = 0;
    uint32_t invalidPixelCount = 0;
};

struct GpuRenderGraphNode {
    ForensicPassId passId = ForensicPassId::Clear;
    std::string passName;
    bool executed = false;
    bool enabled = true;
    double cpuMs = 0.0;
    double gpuMs = 0.0;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::string producedTarget;
};

struct GpuRenderGraphEdge {
    std::string fromPass;
    std::string toPass;
    std::string viaResource;
};

struct GpuFrameComparisonEntry {
    std::string label;
    std::string before;
    std::string after;
    bool changed = false;
};

struct GpuInvestigationReport {
    uint64_t frameNumber = 0;
    glm::vec2 probeUV{0.5f, 0.5f};
    uint32_t probePixelX = 0;
    uint32_t probePixelY = 0;
    float probeDepth = 0.0f;

    std::vector<GpuRenderTargetInfo> renderTargets;
    std::vector<GpuPassValidation> passValidations;
    std::vector<GpuPixelPipelineStage> pixelPipeline;
    std::vector<GpuLutInspection> lutInspections;
    std::vector<GpuRenderGraphNode> graphNodes;
    std::vector<GpuRenderGraphEdge> graphEdges;
    std::vector<std::string> validationWarnings;
    RenderDebuggerFirstFailure firstFailure{};

    AtmosphereRadianceStageSample shaderTrace{};
    bool shaderTraceValid = false;

    bool hasBaselineCapture = false;
    bool hasComparisonCapture = false;
    std::vector<GpuFrameComparisonEntry> frameComparisons;

    int selectedPreviewTargetIndex = -1;
};

class RenderGpuInvestigator {
public:
    RENDERER_API static RenderGpuInvestigator& Get();

    RENDERER_API void SetProbeFromViewportUV(float u, float v, uint32_t bufferWidth, uint32_t bufferHeight);
    RENDERER_API void SetProbePixel(uint32_t x, uint32_t y, uint32_t bufferWidth, uint32_t bufferHeight);
    RENDERER_API glm::vec2 GetProbeUV() const { return m_ProbeUV; }

    RENDERER_API void RequestBaselineCapture();
    RENDERER_API void RequestComparisonCapture();
    RENDERER_API void SelectPreviewTarget(int index) { m_SelectedPreviewTarget = index; }
    RENDERER_API int GetSelectedPreviewTarget() const { return m_SelectedPreviewTarget; }

    RENDERER_API GpuInvestigationReport BuildReport(
        const ForensicFrameReport& forensicReport,
        const std::vector<GpuResourceCatalogEntry>& catalog,
        const std::vector<RenderDebuggerLUTStats>& lutStats,
        const std::vector<GpuCachedLUTData>& cachedLuts,
        const Renderer::CameraUniform& camera,
        const glm::vec3& cameraForward,
        const SceneEnvironmentUniform& env,
        float maxHdrComponent,
        float probeDepth);

    RENDERER_API const GpuInvestigationReport& GetLastReport() const { return m_LastReport; }

    RENDERER_API static std::vector<uint8_t> BuildDownscaledPreview(
        const std::vector<float>& rgba,
        uint32_t width,
        uint32_t height,
        uint32_t maxDim,
        uint32_t& outWidth,
        uint32_t& outHeight);

    RENDERER_API static std::array<uint32_t, 64> BuildLuminanceHistogram(
        const std::vector<float>& rgba,
        uint32_t width,
        uint32_t height);

private:
    RenderGpuInvestigator() = default;

    void BuildRenderGraph(GpuInvestigationReport& report) const;
    void BuildPixelPipeline(
        GpuInvestigationReport& report,
        const ForensicFrameReport& forensicReport,
        const Renderer::CameraUniform& camera,
        const glm::vec3& cameraForward,
        const SceneEnvironmentUniform& env) const;
    void ValidatePasses(GpuInvestigationReport& report, float maxHdrComponent) const;
    void DetectFirstFailure(GpuInvestigationReport& report, const ForensicFrameReport& forensicReport) const;
    void BuildLutInspections(
        GpuInvestigationReport& report,
        const std::vector<RenderDebuggerLUTStats>& lutStats,
        const std::vector<GpuCachedLUTData>& cachedLuts) const;
    void MergeCatalogIntoTargets(
        GpuInvestigationReport& report,
        const std::vector<GpuResourceCatalogEntry>& catalog,
        const ForensicFrameReport& forensicReport) const;
    void CompareWithBaseline(GpuInvestigationReport& report);
    bool IsPixelInvalid(const ForensicPixelRGBA& px, bool isLdr, float maxHdr) const;

    glm::vec2 m_ProbeUV{0.5f, 0.5f};
    uint32_t m_ProbePixelX = 0;
    uint32_t m_ProbePixelY = 0;
    int m_SelectedPreviewTarget = -1;
    bool m_RequestBaseline = false;
    bool m_RequestComparison = false;
    GpuInvestigationReport m_LastReport{};
    GpuInvestigationReport m_BaselineReport{};
    bool m_HasBaseline = false;
};

#endif
} // namespace we::runtime::renderer
