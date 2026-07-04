#include "Environment/EnvironmentSkyAtmosphere.h"

namespace we::runtime::world::environment {

void EnvironmentSkyAtmosphere::ApplyDefaults() {
    RayleighScattering = 0.005802f;
    MieScattering = 0.003996f;
    MieAnisotropy = 0.76f;
    MultiScatterStrength = 1.0f;
    EyeAltitude = 0.001f;
    OzoneAbsorption = glm::vec3(0.00065f, 0.0018f, 0.00008f);
    AerialPerspectiveStartDepth = 0.1f;
    GroundAlbedo = glm::vec3(0.4f, 0.4f, 0.4f);
}

glm::vec3 EnvironmentSkyAtmosphere::GetRayleighColor() const {
    return glm::vec3(
        RayleighScattering * 5.6f,
        RayleighScattering * 13.2f,
        RayleighScattering * 32.0f);
}

glm::vec3 EnvironmentSkyAtmosphere::GetOzoneAbsorption() const {
    return OzoneAbsorption;
}

} // namespace we::runtime::world::environment
