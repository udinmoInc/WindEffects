#pragma once

#include "World/Export.h"

#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentHeightFog.h"
#include "Environment/EnvironmentSkyAtmosphere.h"
#include "Environment/EnvironmentSkyLight.h"
#include "Core/Math/Types.h"

namespace we::runtime::world::environment {

// Orchestrates time-of-day driven updates across Sun, SkyLight, Fog, and Atmosphere.
class WORLD_API EnvironmentManager {
public:
    void UpdateDerivedState(
        EnvironmentDirectionalLight& sun,
        EnvironmentSkyLight& skyLight,
        EnvironmentHeightFog& fog,
        EnvironmentSkyAtmosphere& atmosphere,
        const we::math::Vec3& cameraPosition);

    we::math::Vec3 GetWorldOrigin(const we::math::Vec3& cameraPosition) const;
    float ComputeExposureEV(const EnvironmentDirectionalLight& sun) const;
    float ComputeHdrSkyLuminance(const EnvironmentDirectionalLight& sun, const EnvironmentSkyAtmosphere& atmosphere) const;

private:
    we::math::Vec3 ComputeSkyLightUpper(const EnvironmentDirectionalLight& sun, const EnvironmentSkyAtmosphere& atmosphere) const;
    we::math::Vec3 ComputeSkyLightLower(const EnvironmentDirectionalLight& sun, const EnvironmentHeightFog& fog, const EnvironmentSkyAtmosphere& atmosphere) const;
    we::math::Vec3 ComputeFogColor(const EnvironmentDirectionalLight& sun, const EnvironmentSkyAtmosphere& atmosphere) const;
};

} // namespace we::runtime::world::environment
