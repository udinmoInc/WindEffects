#include "Environment/EnvironmentHeightFog.h"

#include <algorithm>

namespace we::runtime::world::environment {

void EnvironmentHeightFog::ApplyDefaults() {
    Density = 0.02f;
    HeightFalloff = 0.2f;
    StartDistance = 0.0f;
    VolumetricFog = true;
    FogColor = we::math::Vec3(0.72f, 0.78f, 0.85f);
}

void EnvironmentHeightFog::SyncFromEntity(const we::math::Vec4& color, const we::math::Vec3& scale) {
    FogColor = we::math::Vec3(color.x, color.y, color.z);
    Density = std::max(0.0f, scale.x - 0.15f);
}

void EnvironmentHeightFog::ApplyToEntity(we::math::Vec4& color, we::math::Vec3& scale) const {
    color = we::math::Vec4(FogColor.x, FogColor.y, FogColor.z, 1.0f);
    scale = we::math::Vec3(0.15f + Density);
}

} // namespace we::runtime::world::environment
