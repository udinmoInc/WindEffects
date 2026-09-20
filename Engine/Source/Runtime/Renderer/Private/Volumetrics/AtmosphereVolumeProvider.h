// ==============================================================================
// WindEffects — Renderer — AtmosphereVolumeProvider
// Atmosphere as a volumetric density source (not a separate lighting pipeline).
// ==============================================================================
#pragma once

#include "Renderer/Volumetrics/IVolumetricProvider.h"
#include "Renderer/Volumetrics/VolumetricFrameUniform.h"

namespace we::runtime::renderer {

class AtmosphereVolumeProvider final : public IVolumetricProvider {
public:
    AtmosphereVolumeProvider() = default;

    [[nodiscard]] VolumetricProviderType GetType() const override {
        return VolumetricProviderType::Atmosphere;
    }
    [[nodiscard]] bool IsEnabled() const override { return m_Enabled; }
    [[nodiscard]] const char* GetName() const override { return "Atmosphere"; }

    void PrepareFrame(const VolumetricPrepareContext& ctx) override;

    void FillFrameUniform(VolumetricFrameUniform& out) const;

    /// Opt-in only. Visible sky must come from ProceduralSky / EnvironmentLighting —
    /// never from this media provider replacing the HDR scene.
    void SetEnabled(bool enabled) { m_EnabledOverride = enabled; }

private:
    bool m_Enabled = false;
    // OFF by default: Rayleigh params are always present with SkyAtmosphere and must
    // not auto-enable a full-screen volumetric wipe over the procedural sky.
    bool m_EnabledOverride = false;
};

} // namespace we::runtime::renderer
