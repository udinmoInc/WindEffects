#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentDirectionalLight {
public:
    std::uint64_t EntityId = 0;

    float Intensity = 10.0f;
    int TemperatureKelvin = 6500;
    bool CastDynamicShadows = true;
    bool AtmosphereSun = true;
    glm::vec3 Rotation{ -45.0f, 35.0f, 0.0f };
    glm::vec3 Color{ 1.0f, 0.96f, 0.86f };

    void ApplyDefaults();
    glm::vec3 GetLightDirection() const;
    glm::vec3 GetColorFromTemperature() const;
    void SyncFromEntityTransform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec4& color);
    void ApplyToEntity(glm::vec3& position, glm::vec3& rotation, glm::vec4& color) const;
};

} // namespace we::runtime::world::environment
