// ==============================================================================
// WindEffects — Renderer — LightingHierarchy
// Documents the modular lighting stack (not a mega-class).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

// Hierarchy (modules, not inheritance):
//
//   LightingSystem                 — frame orchestration, GPU light buffers
//     ├── DirectLighting           — LightingTypes / GPU*Light buffers
//     ├── EnvironmentLighting      — EnvironmentLightingContext + Evaluator
//     │     └── Atmosphere/Sky     — Rayleigh/Mie driven radiance & irradiance
//     ├── IndirectLighting         — IIndirectLightingProvider (GI plug-in)
//     ├── Visibility               — IShadowVisibilityProvider
//     │     └── ShadowSystem       — Cascaded Shadow Maps (CSM); RT-ready swap
//     ├── SurfaceLighting          — Lighting.hlsli / PBR + Shadow.hlsli
//     └── VolumetricLighting       — VolumetricLighting.hlsli + VolumeProviders
//
// Shared data contracts:
//   SceneEnvironmentUniform        — GPU UBO (sun, atmosphere, exposure, fog)
//   EnvironmentLightingContext     — CPU scene-referred env lighting
//   ShadowCascadeUniform           — CSM light-space matrices / splits
//   VolumetricLightingContext      — volumetric kernel light inputs
//   VolumeSample                   — density/extinction/scattering/anisotropy
//
// HDR → Exposure (EnvironmentExposureController / exposureEV) → TonemapPass

#include "Lighting/EnvironmentLightingContext.h"
#include "Lighting/EnvironmentLightingEvaluator.h"
#include "Lighting/IIndirectLightingProvider.h"
#include "Lighting/IShadowVisibilityProvider.h"
#include "Lighting/LightingSystem.h"
#include "Lighting/VolumetricLightingContext.h"
