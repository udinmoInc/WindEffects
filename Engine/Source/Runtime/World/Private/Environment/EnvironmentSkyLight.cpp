#include "Environment/EnvironmentSkyLight.h"

#include <algorithm>

#include "Core/Math/GlmInterop.h"
namespace we::runtime::world::environment {

void EnvironmentSkyLight::ApplyDefaults() {
    Intensity = 1.0f;
    RealTimeCapture = true;
    LowerHemisphereColor = we::math::Vec3(0.05f, 0.05f, 0.06f);
    UpperHemisphereColor = we::math::Vec3(0.05f, 0.08f, 0.12f);
}

we::math::Vec3 EnvironmentSkyLight::GetAmbientColor() const {
    return UpperHemisphereColor * Intensity;
}

void EnvironmentSkyLight::SyncFromEntity(const we::math::Vec4& color) {
    UpperHemisphereColor = we::math::Vec3(color.x, color.y, color.z) / std::max(Intensity, 0.001f);
}

void EnvironmentSkyLight::ApplyToEntity(we::math::Vec4& color) const {
    color = we::math::Vec4(GetAmbientColor(), 1.0f);
}

} // namespace we::runtime::world::environment
