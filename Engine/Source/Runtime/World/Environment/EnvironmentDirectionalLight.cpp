#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentLighting.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

void EnvironmentDirectionalLight::ApplyDefaults() {
    Intensity = 10.0f;
    TemperatureKelvin = 6500;
    CastDynamicShadows = true;
    AtmosphereSun = true;
    Rotation = glm::vec3(-45.0f, 35.0f, 0.0f);
    Color = TemperatureKelvinToRgb(TemperatureKelvin);
}

glm::vec3 EnvironmentDirectionalLight::GetLightDirection() const {
    return EulerDegreesToLightDirection(Rotation);
}

glm::vec3 EnvironmentDirectionalLight::GetColorFromTemperature() const {
    return TemperatureKelvinToRgb(TemperatureKelvin);
}

void EnvironmentDirectionalLight::SyncFromEntityTransform(
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec4& color) {
    (void)position;
    Rotation = rotation;
    Color = glm::vec3(color);
}

void EnvironmentDirectionalLight::ApplyToEntity(glm::vec3& position, glm::vec3& rotation, glm::vec4& color) const {
    position = glm::vec3(0.0f, 24.0f, 0.0f);
    rotation = Rotation;
    color = glm::vec4(GetColorFromTemperature(), 1.0f);
}

} // namespace we::runtime::world::environment
