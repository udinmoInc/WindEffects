// ==============================================================================
// WindEffects — Renderer — CloudVolumeProvider
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Volumetrics/CloudVolumeProvider.h"

namespace we::runtime::renderer {

void CloudVolumeProvider::PrepareFrame(const VolumetricPrepareContext& ctx) {
    m_Cloud = ctx.cloud;
    if (!m_EnabledOverride || !m_Cloud) {
        m_Enabled = false;
        return;
    }
    m_Enabled = m_Cloud->enabled >= 0.5f;
}

} // namespace we::runtime::renderer
