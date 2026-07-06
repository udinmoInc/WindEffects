#pragma once

#include "Renderer/Export.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"

#if WE_HAS_GLM
#include <glm/glm.hpp>
#include <string>
#include <vector>
#endif

namespace we::runtime::renderer {

#if WE_HAS_GLM

struct AtmosphereRadianceStageSample {
    std::string label;
    glm::vec2 screenUV{ 0.5f, 0.5f };

    glm::vec3 viewDirection{ 0.0f, 1.0f, 0.0f };
    float viewDirectionLength = 1.0f;
    glm::vec3 sunDirection{ 0.0f, 1.0f, 0.0f };
    float sunDirectionLength = 1.0f;
    float viewSunDot = 0.0f;

    float atmosphereHeightKm = 0.0f;

    glm::vec2 transmittanceUV{ 0.0f };
    glm::vec3 transmittanceRGB{ 1.0f, 1.0f, 1.0f };

    glm::vec2 skyViewUV{ 0.0f };
    glm::vec3 skyViewRGB{ 0.0f };

    glm::vec3 rayleighRGB{ 0.0f };
    glm::vec3 mieRGB{ 0.0f };
    glm::vec3 multiScatterRGB{ 0.0f };
    glm::vec3 sunDiskRGB{ 0.0f };
    glm::vec3 finalHdrRGB{ 0.0f };

    float sunDiskMask = 0.0f;
    bool valid = false;
};

struct AtmosphereRadianceValidationFailure {
    std::string stage;
    std::string function;
    std::string sourceFile;
    int sourceLine = 0;
    std::string variable;
    std::string previousValid;
    std::string invalidValue;
    std::string operation;
};

class AtmosphereRadianceTracer {
public:
    RENDERER_API static AtmosphereRadianceTracer& Get();

    RENDERER_API AtmosphereRadianceStageSample TraceSample(
        const std::string& label,
        const glm::vec2& screenUV,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& cameraPos,
        const SceneEnvironmentUniform& env) const;

    RENDERER_API std::vector<AtmosphereRadianceStageSample> TraceStandardSamples(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        const glm::vec3& cameraRight,
        const glm::vec3& cameraUp,
        const SceneEnvironmentUniform& env) const;

    RENDERER_API bool ValidateSample(
        const AtmosphereRadianceStageSample& sample,
        AtmosphereRadianceValidationFailure& outFailure) const;

    RENDERER_API std::string FormatSampleReport(const AtmosphereRadianceStageSample& sample) const;
    RENDERER_API std::string FormatFailureReport(const AtmosphereRadianceValidationFailure& failure) const;

    RENDERER_API void LogStandardTrace(
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& cameraPos,
        const glm::vec3& cameraForward,
        const glm::vec3& cameraRight,
        const glm::vec3& cameraUp,
        const SceneEnvironmentUniform& env) const;

private:
    AtmosphereRadianceTracer() = default;
};

#endif // WE_HAS_GLM

} // namespace we::runtime::renderer
