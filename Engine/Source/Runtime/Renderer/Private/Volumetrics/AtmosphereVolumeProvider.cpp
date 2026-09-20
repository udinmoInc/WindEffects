// ==============================================================================
// WindEffects — Renderer — AtmosphereVolumeProvider
// ==============================================================================
#include "Volumetrics/AtmosphereVolumeProvider.h"

namespace we::runtime::renderer {

void AtmosphereVolumeProvider::PrepareFrame(const VolumetricPrepareContext& ctx) {
    m_Enabled = false;
    // Visible sky / environment radiance is owned by EnvironmentLighting → ProceduralSky.
    // Volumetric atmosphere is aerial-perspective media and must be explicitly enabled.
    if (!m_EnabledOverride || !ctx.environment) {
        return;
    }
    const SceneEnvironmentUniform& env = *ctx.environment;
    const float rayleighMag =
        env.atmosphereRayleigh.x + env.atmosphereRayleigh.y + env.atmosphereRayleigh.z;
    m_Enabled = rayleighMag > 1.0e-8f && env.atmosphereHeight > 0.0f;
}

void AtmosphereVolumeProvider::FillFrameUniform(VolumetricFrameUniform& out) const {
    // reserved0 = atmosphere media enable (shader provider mask also gates).
    out.reserved0 = m_Enabled ? 1.0f : 0.0f;
}

} // namespace we::runtime::renderer
