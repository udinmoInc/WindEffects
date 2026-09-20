// ==============================================================================
// WindEffects — Renderer — ShadowCascadeUniform
// GPU layout for CSM (std140). Must match ShadowBuffer.hlsli.
// ==============================================================================
#pragma once

#include "Core/Math/Types.h"
#include "Renderer/Export.h"

#include <cstdint>

namespace we::runtime::renderer {

inline constexpr uint32_t kMaxShadowCascades = 4;

/// std140 — keep in sync with Engine/Shaders/Common/ShadowBuffer.hlsli
struct ShadowCascadeUniform {
    we::math::Mat4 lightViewProj[kMaxShadowCascades]{};
    float cascadeSplits[4]{};          // view-space split distances (ascending)
    float atlasScaleBias[kMaxShadowCascades][4]{}; // xy scale, zw offset (UV)
    float sunTravel[4]{};              // xyz travel dir, w = castsShadows
    float params0[4]{}; // depthBias, normalBiasMeters, filterRadiusTexels, cascadeCount
    float params1[4]{}; // cascadeBlend, softShadows, contactShadows, contactLength
    float params2[4]{}; // atlasRes, cascadeRes, enabled, shadowDistance
};

static_assert(sizeof(ShadowCascadeUniform) == 64 * 4 + 16 + 64 + 16 + 16 + 16 + 16,
    "ShadowCascadeUniform size drift — update ShadowBuffer.hlsli");

} // namespace we::runtime::renderer
