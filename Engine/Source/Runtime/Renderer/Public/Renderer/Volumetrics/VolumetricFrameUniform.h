// ==============================================================================
// WindEffects — Renderer — VolumetricFrameUniform
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstddef>
#include <cstdint>

namespace we::runtime::renderer {

/// Shared volumetric pipeline UBO (std140 / float4 rows). ~128 bytes.
struct VolumetricFrameUniform {
    uint32_t maxSteps = 128;
    uint32_t lightSteps = 6;
    uint32_t shadowSteps = 4;
    uint32_t frameIndex = 0;

    float resolutionScale = 1.0f;
    float temporalBlend = 0.85f;
    float jitterX = 0.0f;
    float jitterY = 0.0f;

    uint32_t providerMask = 0;
    uint32_t temporalQuality = 1;
    uint32_t shadowQuality = 1;
    uint32_t lightingQuality = 1;

    float heightFogDensity = 0.0f;
    float heightFogFalloff = 0.2f;
    float heightFogStart = 0.0f;
    float heightFogEnabled = 0.0f;

    // localFogCenter.xyz + density
    float localFogCenterX = 0.0f;
    float localFogCenterY = 0.0f;
    float localFogCenterZ = 0.0f;
    float localFogDensity = 0.0f;

    // localFogHalfExtents.xyz + anisotropy
    float localFogHalfExtentX = 10.0f;
    float localFogHalfExtentY = 5.0f;
    float localFogHalfExtentZ = 10.0f;
    float localFogAnisotropy = 0.0f;

    // localFogAlbedo.rgb + enabled
    float localFogAlbedoR = 0.7f;
    float localFogAlbedoG = 0.75f;
    float localFogAlbedoB = 0.85f;
    float localFogEnabled = 0.0f;

    // Reserved float4 — keeps UBO at 128 bytes for future expansion.
    float reserved0 = 0.0f;
    float reserved1 = 0.0f;
    float reserved2 = 0.0f;
    float reserved3 = 0.0f;
};

inline constexpr std::size_t kVolumetricFrameUniformSize = 128;
static_assert(sizeof(VolumetricFrameUniform) == kVolumetricFrameUniformSize,
    "VolumetricFrameUniform must match HLSL std140 packing (128 bytes).");

} // namespace we::runtime::renderer
