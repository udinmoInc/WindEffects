#include "Environment/EnvironmentHeightFog.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentHeightFog::ApplyDefaults() {
    Density = 0.02f;
    HeightFalloff = 0.2f;
    StartDistance = 0.0f;
    VolumetricFog = true;
    FogColor = glm::vec3(0.72f, 0.78f, 0.85f);
}

void EnvironmentHeightFog::SyncFromEntity(const glm::vec4& color, const glm::vec3& scale) {
    FogColor = glm::vec3(color);
    Density = std::max(0.0f, scale.x - 0.15f);
}

void EnvironmentHeightFog::ApplyToEntity(glm::vec4& color, glm::vec3& scale) const {
    color = glm::vec4(FogColor, 1.0f);
    scale = glm::vec3(0.15f + Density);
}

} // namespace we::runtime::world::environment
