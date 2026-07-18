#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentLighting.h"

#include "Core/Math/GlmInterop.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::world::environment {

void EnvironmentDirectionalLight::ApplyDefaults() {
    Intensity = 10.0f;
    TemperatureKelvin = 6500;
    CastDynamicShadows = true;
    AtmosphereSun = true;
    Rotation = we::math::Vec3(-45.0f, 35.0f, 0.0f);
    Color = TemperatureKelvinToRgb(TemperatureKelvin);
}

we::math::Vec3 EnvironmentDirectionalLight::GetLightDirection() const {
    return EulerDegreesToLightDirection(Rotation);
}

we::math::Vec3 EnvironmentDirectionalLight::GetColorFromTemperature() const {
    return TemperatureKelvinToRgb(TemperatureKelvin);
}

void EnvironmentDirectionalLight::SyncFromEntityTransform(
    const we::math::Vec3& position,
    const we::math::Vec3& rotation,
    const we::math::Vec4& color) {
    (void)position;
    Rotation = rotation;
    Color = we::math::Vec3(color.x, color.y, color.z);
}

void EnvironmentDirectionalLight::ApplyToEntity(we::math::Vec3& position, we::math::Vec3& rotation, we::math::Vec4& color) const {
    position = we::math::Vec3(0.0f, 24.0f, 0.0f);
    rotation = Rotation;
    const we::math::Vec3 c = GetColorFromTemperature();
    color = we::math::Vec4(c.x, c.y, c.z, 1.0f);
}

} // namespace we::runtime::world::environment
