// ==============================================================================
// WindEffects — Renderer — LightingTypes
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

struct DirectionalLight {
    we::math::Vec3 direction{0.3f, -0.8f, 0.2f};
    we::math::Vec3 color{1.0f, 0.98f, 0.95f};
    float intensity = 1.2f;
    bool enabled = true;
    bool castsShadows = true;
};

struct PointLight {
    we::math::Vec3 position{0.0f, 0.0f, 0.0f};
    we::math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    bool enabled = true;
};

struct SpotLight {
    we::math::Vec3 position{0.0f, 0.0f, 0.0f};
    we::math::Vec3 direction{0.0f, -1.0f, 0.0f};
    we::math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 20.0f;
    float outerConeDegrees = 30.0f;
    bool enabled = true;
};

struct EnvironmentLighting {
    we::math::Vec3 ambientUpper{0.35f, 0.42f, 0.55f};
    we::math::Vec3 ambientLower{0.15f, 0.16f, 0.18f};
    float skyLightIntensity = 1.0f;
};

/// GPU layout — std140-friendly (Vec4 slots). Consumed by future PBR shaders.
struct GPUDirectionalLight {
    float direction[4]{};      // xyz, castsShadows as w
    float colorIntensity[4]{}; // rgb, intensity as w
};

struct GPUPointLight {
    float positionRange[4]{};  // xyz, range as w
    float colorIntensity[4]{}; // rgb, intensity as w
};

struct GPUSpotLight {
    float positionRange[4]{};
    float directionCone[4]{};  // xyz, outerConeCos as w
    float colorIntensity[4]{}; // rgb, intensity as w
    float innerConePad[4]{};   // x = innerConeCos
};

struct GPULightingConstants {
    uint32_t directionalCount = 0;
    uint32_t pointCount = 0;
    uint32_t spotCount = 0;
    uint32_t maxLocalLights = 64;
    float ambientUpper[4]{};
    float ambientLower[4]{};
    float skyLightIntensity = 1.0f;
    float _pad0 = 0.0f;
    float _pad1 = 0.0f;
    float _pad2 = 0.0f;
};

constexpr uint32_t kMaxDirectionalLights = 4;
constexpr uint32_t kMaxSpotLights = 64;

} // namespace we::runtime::renderer
