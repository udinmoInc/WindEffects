#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentSkyAtmosphere {
public:
    std::uint64_t EntityId = 0;

    float RayleighScattering = 0.005802f;
    float MieScattering = 0.003996f;
    float MieAnisotropy = 0.76f;
    float MultiScatterStrength = 1.0f;
    float EyeAltitude = 0.001f;
    glm::vec3 OzoneAbsorption{ 0.00065f, 0.0018f, 0.00008f };
    float AerialPerspectiveStartDepth = 0.1f;
    glm::vec3 GroundAlbedo{ 0.4f, 0.4f, 0.4f };
    int AtmosphereDebugMode = 0;

    void ApplyDefaults();
    glm::vec3 GetRayleighColor() const;
    glm::vec3 GetOzoneAbsorption() const;
};

} // namespace we::runtime::world::environment
