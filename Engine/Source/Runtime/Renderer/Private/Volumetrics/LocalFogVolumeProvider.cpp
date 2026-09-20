// ==============================================================================
// WindEffects — Renderer — LocalFogVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Volumetrics/LocalFogVolumeProvider.h"

namespace we::runtime::renderer {

void LocalFogVolumeProvider::PrepareFrame(const VolumetricPrepareContext& ctx) {
    if (ctx.localFog) {
        m_LocalFog = *ctx.localFog;
    }
    m_Enabled = m_LocalFog.enabled >= 0.5f;
}

void LocalFogVolumeProvider::FillFrameUniform(VolumetricFrameUniform& out) const {
    out.localFogCenterX = m_LocalFog.center.x;
    out.localFogCenterY = m_LocalFog.center.y;
    out.localFogCenterZ = m_LocalFog.center.z;
    out.localFogDensity = m_LocalFog.density;

    out.localFogHalfExtentX = m_LocalFog.halfExtents.x;
    out.localFogHalfExtentY = m_LocalFog.halfExtents.y;
    out.localFogHalfExtentZ = m_LocalFog.halfExtents.z;
    out.localFogAnisotropy = m_LocalFog.anisotropy;

    out.localFogAlbedoR = m_LocalFog.albedo.x;
    out.localFogAlbedoG = m_LocalFog.albedo.y;
    out.localFogAlbedoB = m_LocalFog.albedo.z;
    out.localFogEnabled = m_Enabled ? 1.0f : 0.0f;
}

} // namespace we::runtime::renderer
