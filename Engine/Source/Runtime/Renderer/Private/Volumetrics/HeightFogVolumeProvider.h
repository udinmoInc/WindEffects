// ==============================================================================
// WindEffects — Renderer — HeightFogVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Volumetrics/IVolumetricProvider.h"
#include "Renderer/Volumetrics/VolumetricFrameUniform.h"

namespace we::runtime::renderer {

/// Height fog from SceneEnvironmentUniform fog fields.
class HeightFogVolumeProvider final : public IVolumetricProvider {
public:
    HeightFogVolumeProvider() = default;

    [[nodiscard]] VolumetricProviderType GetType() const override {
        return VolumetricProviderType::HeightFog;
    }
    [[nodiscard]] bool IsEnabled() const override { return m_Enabled; }
    [[nodiscard]] const char* GetName() const override { return "HeightFog"; }

    void PrepareFrame(const VolumetricPrepareContext& ctx) override;

    void FillFrameUniform(VolumetricFrameUniform& out) const;

    void SetEnabled(bool enabled) { m_EnabledOverride = enabled; }

    [[nodiscard]] float Density() const { return m_Density; }
    [[nodiscard]] float Falloff() const { return m_Falloff; }
    [[nodiscard]] float StartDistance() const { return m_StartDistance; }

private:
    bool m_Enabled = false;
    bool m_EnabledOverride = true;
    float m_Density = 0.0f;
    float m_Falloff = 0.2f;
    float m_StartDistance = 0.0f;
};

} // namespace we::runtime::renderer
