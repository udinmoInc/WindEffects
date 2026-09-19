// ==============================================================================
// WindEffects — Renderer — CloudUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>

namespace we::runtime::renderer {

// Must match CloudBuffer in VolumetricClouds.hlsl (std140-ish float packing).
struct CloudUniform {
    float planetRadius = 6371000.0f;   // meters
    float innerOffset = 1500.0f;
    float outerOffset = 4000.0f;
    float coverage = 0.62f;

    float cloudType = 0.45f;
    float precipitation = 0.0f;
    float timeSeconds = 0.0f;
    float densityScale = 0.11f;

    float windDirX = 1.0f;
    float windDirY = 0.0f;
    float windDirZ = 0.12f;
    float windSpeed = 22.0f;

    float baseScale = 0.00015f;
    float detailScale = 0.0015f;
    float curlStrength = 0.45f;
    // Light-march sigma (Schneider). View extinction is derived higher in the PS.
    float absorption = 0.08f;

    float ambient = 0.65f;
    float silverIntensity = 0.70f;
    float silverSpread = 4.5f;
    float powderStrength = 0.55f;

    float phaseGForward = 0.75f;
    float phaseGBack = -0.25f;
    uint32_t maxSteps = 128;
    uint32_t lightSteps = 6;

    float enabled = 1.0f;
    float lightMarchLength = 800.0f;
    float albedo = 0.95f;
    float padding = 0.0f;
};

static_assert(sizeof(CloudUniform) == 112, "CloudUniform packing must match HLSL CloudBuffer.");

} // namespace we::runtime::renderer
