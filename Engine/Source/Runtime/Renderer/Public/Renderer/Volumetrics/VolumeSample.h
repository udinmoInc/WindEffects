// ==============================================================================
// WindEffects — Renderer — VolumeSample
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

namespace we::runtime::renderer {

/// CPU-side volume sample result (density / optical properties).
struct VolumeSample {
    float density = 0.0f;
    float extinction = 0.0f;
    float scattering = 0.0f;
    float anisotropy = 0.0f;
};

} // namespace we::runtime::renderer
