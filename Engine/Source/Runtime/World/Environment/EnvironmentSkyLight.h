#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentSkyLight {
public:
    std::uint64_t EntityId = 0;

    float Intensity = 1.0f;
    bool RealTimeCapture = true;
    glm::vec3 LowerHemisphereColor{ 0.05f, 0.05f, 0.06f };
    glm::vec3 UpperHemisphereColor{ 0.05f, 0.08f, 0.12f };

    void ApplyDefaults();
    glm::vec3 GetAmbientColor() const;
    void SyncFromEntity(const glm::vec4& color);
    void ApplyToEntity(glm::vec4& color) const;
};

} // namespace we::runtime::world::environment
