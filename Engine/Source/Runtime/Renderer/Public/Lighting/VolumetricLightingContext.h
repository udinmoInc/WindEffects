// ==============================================================================
// WindEffects — Renderer — VolumetricLightingContext
// Lighting inputs for the unified volumetric kernel (sun / env / locals).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

#include "Lighting/EnvironmentLightingContext.h"
#include "Lighting/LightingTypes.h"
#include "Renderer/Export.h"

#include <cstdint>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

struct RENDERER_API VolumetricLightingContext {
    EnvironmentLightingContext environment{};

    DirectionalLight primaryDirectional{};

    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;
    uint32_t spotCount = 0;

    float multiScatterStrength = 1.0f;
    float environmentScatterScale = 1.0f;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
