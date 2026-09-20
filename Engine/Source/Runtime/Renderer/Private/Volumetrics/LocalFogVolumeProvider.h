// ==============================================================================
// WindEffects — Renderer — LocalFogVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Volumetrics/IVolumetricProvider.h"
#include "Renderer/Volumetrics/LocalFogUniform.h"
#include "Renderer/Volumetrics/VolumetricFrameUniform.h"

namespace we::runtime::renderer {

/// Local box fog volume. Owns LocalFogUniform; default disabled.
class LocalFogVolumeProvider final : public IVolumetricProvider {
public:
    LocalFogVolumeProvider() = default;

    [[nodiscard]] VolumetricProviderType GetType() const override {
        return VolumetricProviderType::LocalFog;
    }
    [[nodiscard]] bool IsEnabled() const override { return m_Enabled; }
    [[nodiscard]] const char* GetName() const override { return "LocalFog"; }

    void PrepareFrame(const VolumetricPrepareContext& ctx) override;

    void FillFrameUniform(VolumetricFrameUniform& out) const;

    void SetLocalFog(const LocalFogUniform& fog) { m_LocalFog = fog; }
    [[nodiscard]] LocalFogUniform& GetLocalFog() { return m_LocalFog; }
    [[nodiscard]] const LocalFogUniform& GetLocalFog() const { return m_LocalFog; }

private:
    bool m_Enabled = false;
    LocalFogUniform m_LocalFog{};
};

} // namespace we::runtime::renderer
