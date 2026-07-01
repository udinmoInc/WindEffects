#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentVolumetricClouds {
public:
    std::uint64_t EntityId = 0;

    bool Enabled = false;
    float Coverage = 0.45f;
    float Altitude = 5000.0f;
    float Extinction = 0.35f;
    glm::vec3 CloudColor{ 0.95f, 0.96f, 0.98f };

    void ApplyDefaults();
};

} // namespace we::runtime::world::environment
