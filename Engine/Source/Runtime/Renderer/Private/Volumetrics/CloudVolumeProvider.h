// ==============================================================================
// WindEffects — Renderer — CloudVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Lighting/CloudUniform.h"
#include "Renderer/Volumetrics/IVolumetricProvider.h"

namespace we::runtime::renderer {

/// Cloud provider — mirrors CloudUniform enable state; no RHI ownership.
class CloudVolumeProvider final : public IVolumetricProvider {
public:
    CloudVolumeProvider() = default;

    [[nodiscard]] VolumetricProviderType GetType() const override {
        return VolumetricProviderType::Cloud;
    }
    [[nodiscard]] bool IsEnabled() const override { return m_Enabled; }
    [[nodiscard]] const char* GetName() const override { return "Cloud"; }

    void PrepareFrame(const VolumetricPrepareContext& ctx) override;

    [[nodiscard]] const CloudUniform* GetCloudUniform() const { return m_Cloud; }

    void SetEnabled(bool enabled) { m_EnabledOverride = enabled; }

private:
    bool m_Enabled = false;
    bool m_EnabledOverride = true;
    const CloudUniform* m_Cloud = nullptr;
};

} // namespace we::runtime::renderer
