// ==============================================================================
// WindEffects — World — EnvironmentSkyLight
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
