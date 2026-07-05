#include "Environment/EnvironmentSkyLight.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentSkyLight::ApplyDefaults() {
    Intensity = 1.0f;
    RealTimeCapture = true;
    LowerHemisphereColor = glm::vec3(0.05f, 0.05f, 0.06f);
    UpperHemisphereColor = glm::vec3(0.05f, 0.08f, 0.12f);
}

glm::vec3 EnvironmentSkyLight::GetAmbientColor() const {
    return UpperHemisphereColor * Intensity;
}

void EnvironmentSkyLight::SyncFromEntity(const glm::vec4& color) {
    UpperHemisphereColor = glm::vec3(color) / std::max(Intensity, 0.001f);
}

void EnvironmentSkyLight::ApplyToEntity(glm::vec4& color) const {
    color = glm::vec4(GetAmbientColor(), 1.0f);
}

} // namespace we::runtime::world::environment
