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
    float AerialPerspectiveStartDepth = 0.1f;
    glm::vec3 GroundAlbedo{ 0.4f, 0.4f, 0.4f };

    void ApplyDefaults();
    glm::vec3 GetRayleighColor() const;
};

} // namespace we::runtime::world::environment
