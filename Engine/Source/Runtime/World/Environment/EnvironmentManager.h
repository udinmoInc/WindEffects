#pragma once

#include "EnvironmentDirectionalLight.h"
#include "EnvironmentHeightFog.h"
#include "EnvironmentSkyAtmosphere.h"
#include "EnvironmentSkyLight.h"
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

// Orchestrates time-of-day driven updates across Sun, SkyLight, Fog, and Atmosphere.
class EnvironmentManager {
public:
    void UpdateDerivedState(
        EnvironmentDirectionalLight& sun,
        EnvironmentSkyLight& skyLight,
        EnvironmentHeightFog& fog,
        EnvironmentSkyAtmosphere& atmosphere,
        const glm::vec3& cameraPosition);

    glm::vec3 GetWorldOrigin(const glm::vec3& cameraPosition) const;
    float ComputeExposureEV(const EnvironmentDirectionalLight& sun) const;

private:
    glm::vec3 ComputeSkyLightUpper(const EnvironmentDirectionalLight& sun, const EnvironmentSkyAtmosphere& atmosphere) const;
    glm::vec3 ComputeSkyLightLower(const EnvironmentDirectionalLight& sun, const EnvironmentHeightFog& fog) const;
    glm::vec3 ComputeFogColor(const EnvironmentDirectionalLight& sun, const EnvironmentSkyAtmosphere& atmosphere) const;
};

} // namespace we::runtime::world::environment
