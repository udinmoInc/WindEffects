#pragma once

#include "Renderer/Export.hpp"
#include <cstdint>
#include <string>
#include <vector>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#include "Renderer/SceneEnvironmentUniform.hpp"
#endif

namespace we::runtime::renderer {

struct FramePassRecord {
    std::string name;
    std::string status = "pending";
    double cpuMs = 0.0;
};

#if WE_HAS_GLM
struct AtmosphereFrameProbe {
    glm::vec3 cameraForward{ 0.0f };
    glm::vec3 viewDirection{ 0.0f, 1.0f, 0.0f };
    float viewDirectionLength = 1.0f;
    glm::vec3 sunDirection{ 0.0f, 1.0f, 0.0f };
    float sunDirectionLength = 1.0f;
    float viewSunDot = 0.0f;
    float viewZenithAngle = 0.0f;
    glm::vec2 skyViewUV{ 0.0f };
    glm::vec2 transmittanceUV{ 0.0f };
    float sunAngularRadius = 0.004675f;
    float sunDiskMask = 0.0f;
    glm::vec3 rayleighRGB{ 0.0f };
    glm::vec3 mieRGB{ 0.0f };
    glm::vec3 multiScatterRGB{ 0.0f };
    glm::vec3 sunRGB{ 0.0f };
    glm::vec3 finalHdrRGB{ 0.0f };
    bool valid = false;
};
#endif

struct FrameStats {
    double cpuFrameMs = 0.0;
    double gpuFrameMs = 0.0;
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t descriptorUpdates = 0;
    double skyPassMs = 0.0;
    double cloudsPassMs = 0.0;
    double scenePassMs = 0.0;
    double fogPassMs = 0.0;
    double postPassMs = 0.0;
    double lutGenerationMs = 0.0;
    bool atmosphereLutReady = false;
    bool skyPipelineValid = false;
    bool postPipelineValid = false;
    std::string lastDiagnosticSummary;
    std::string atmosphereLutStatus = "pending";
    std::string skyPassStatus = "pending";
    std::string cloudsPassStatus = "pending";
    std::string fogPassStatus = "pending";
    std::string postPassStatus = "pending";
    std::vector<FramePassRecord> passExecutions;
    float gpuAverageLuminance = 0.0f;
    bool autoExposureEnabled = true;
    float manualExposureEV = 0.0f;
    float sunDerivedExposureEV = 0.0f;
    float effectiveExposureEV = 0.0f;
#if WE_HAS_GLM
    AtmosphereFrameProbe atmosphereProbe{};
#endif
};

class FrameStatsCollector {
public:
    RENDERER_API static FrameStatsCollector& Get();

    RENDERER_API void BeginFrame();
    RENDERER_API void EndFrame();
    RENDERER_API void AddDrawCall(uint32_t triangles = 0);
    RENDERER_API void AddDescriptorUpdate(uint32_t count = 1);
    RENDERER_API void RecordPassMs(const char* passName, double ms);
    RENDERER_API void RecordPassExecution(const char* passName, const char* status, double cpuMs = 0.0);
    RENDERER_API void SetAtmosphereLutReady(bool ready);
    RENDERER_API void SetSkyPipelineValid(bool valid);
    RENDERER_API void SetPostPipelineValid(bool valid);
    RENDERER_API void SetPassStatus(const char* passName, const char* status);
    RENDERER_API void SetGpuAverageLuminance(float luminance);
    RENDERER_API void SetExposureDiagnostics(
        bool autoExposureEnabled,
        float manualExposureEV,
        float sunDerivedExposureEV,
        float effectiveExposureEV);

#if WE_HAS_GLM
    RENDERER_API void SetAtmosphereProbe(const AtmosphereFrameProbe& probe);
    RENDERER_API AtmosphereFrameProbe ComputeAtmosphereProbe(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        const SceneEnvironmentUniform& env);
#endif

    RENDERER_API const FrameStats& GetStats() const;
    RENDERER_API std::string GetOverlayText() const;

private:
    FrameStats m_Stats{};
    double m_FrameStartMs = 0.0;
};

} // namespace we::runtime::renderer
