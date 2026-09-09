// ==============================================================================
// WindEffects — Renderer — CameraUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>

namespace we::runtime::renderer {

struct CameraUniform {
    we::math::Mat4 view{};
    we::math::Mat4 proj{};
    we::math::Mat4 invViewProj{};
    we::math::Vec3 position{};
    float padding = 0.0f;
};

} // namespace we::runtime::renderer
