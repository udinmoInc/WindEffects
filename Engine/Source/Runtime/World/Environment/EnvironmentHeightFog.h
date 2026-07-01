#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

class EnvironmentHeightFog {
public:
    std::uint64_t EntityId = 0;

    float Density = 0.02f;
    float HeightFalloff = 0.2f;
    float StartDistance = 0.0f;
    bool VolumetricFog = true;
    glm::vec3 FogColor{ 0.72f, 0.78f, 0.85f };

    void ApplyDefaults();
    void SyncFromEntity(const glm::vec4& color, const glm::vec3& scale);
    void ApplyToEntity(glm::vec4& color, glm::vec3& scale) const;
};

} // namespace we::runtime::world::environment
