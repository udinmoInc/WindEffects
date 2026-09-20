// ==============================================================================
// WindEffects — Renderer — LocalFogUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Math/Types.h"

namespace we::runtime::renderer {

/// CPU authoring POD for a local fog volume (box). Default disabled.
struct LocalFogUniform {
    we::math::Vec3 center{0.0f, 0.0f, 0.0f};
    we::math::Vec3 halfExtents{10.0f, 5.0f, 10.0f};
    float density = 0.0f;
    we::math::Vec3 albedo{0.7f, 0.75f, 0.85f};
    float anisotropy = 0.0f;
    float enabled = 0.0f;
};

} // namespace we::runtime::renderer
