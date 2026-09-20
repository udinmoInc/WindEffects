// ==============================================================================
// WindEffects — Renderer — HeightFogVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Volumetrics/HeightFogVolumeProvider.h"

namespace we::runtime::renderer {

void HeightFogVolumeProvider::PrepareFrame(const VolumetricPrepareContext& ctx) {
    m_Density = 0.0f;
    m_Falloff = 0.2f;
    m_StartDistance = 0.0f;
    m_Enabled = false;

    if (!m_EnabledOverride || !ctx.environment) {
        return;
    }

    const SceneEnvironmentUniform& env = *ctx.environment;
    // Artist density is scene-scale; volumetric march uses a gentler optical scale so
    // fog cannot opaque-wipe the procedural sky (baseline must stay visible).
    constexpr float kVolumetricDensityScale = 0.012f;
    m_Density = env.fogDensity * kVolumetricDensityScale;
    m_Falloff = env.fogHeightFalloff;
    m_StartDistance = env.fogStartDistance;

    // Require the explicit volumetric-fog flag. Density alone must not auto-enable
    // a full-screen media pass (default ExponentialHeightFog always has density > 0).
    m_Enabled = env.enableVolumetricFog >= 0.5f && env.fogDensity > 1.0e-6f;
}

void HeightFogVolumeProvider::FillFrameUniform(VolumetricFrameUniform& out) const {
    out.heightFogDensity = m_Density;
    out.heightFogFalloff = m_Falloff;
    out.heightFogStart = m_StartDistance;
    out.heightFogEnabled = m_Enabled ? 1.0f : 0.0f;
}

} // namespace we::runtime::renderer
