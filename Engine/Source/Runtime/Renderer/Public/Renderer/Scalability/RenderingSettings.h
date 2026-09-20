// ==============================================================================
// WindEffects — Renderer — RenderingSettings
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Renderer/Scalability/QualityTypes.h"

#include <cstdint>
#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

/// Configuration-only POD slices. Never hold RHI objects or renderer instances.

struct ResolutionSettings {
    float screenPercentage = 100.0f;
    uint32_t maxShadowMapResolution = 2048;
    uint32_t msaaSamples = 1;
    bool dynamicResolution = false;
};

struct GeometryQualitySettings {
    QualityLevel quality = QualityLevel::High;
    float lodBias = 0.0f;
    bool meshShaders = false;
    FeatureRequirement meshShaderRequirement = FeatureRequirement::Optional;
};

struct TextureQualitySettings {
    QualityLevel quality = QualityLevel::High;
    float anisotropy = 8.0f;
    uint32_t streamingPoolMb = 512;
};

struct LightingQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
    uint32_t maxLocalLights = 64;
};

struct ShadowQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
    uint32_t cascadeCount = 4;
    float resolutionScale = 1.0f;
    bool softShadows = true;
    bool contactShadows = false;
    /// Max view-space distance for cascades (meters).
    float shadowDistance = 2000.0f;
    float shadowNear = 0.1f;
    /// Practical split blend: 0 = uniform, 1 = logarithmic.
    float cascadeSplitLambda = 0.85f;
    /// Blend width as a fraction of each cascade range (seam hiding).
    float cascadeBlend = 0.12f;
    float depthBias = 0.0015f;
    /// Receiver normal offset in meters.
    float normalBias = 0.03f;
    /// PCF kernel radius in shadow-map texels.
    float filterRadius = 1.25f;
    /// Optional short-range contact shadow ray length (meters).
    float contactShadowLength = 0.35f;
};

struct ReflectionQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    bool screenSpace = true;
    bool rayTraced = false;
    FeatureRequirement rayTracingRequirement = FeatureRequirement::Optional;
};

struct GIQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    bool rayTraced = false;
    FeatureRequirement rayTracingRequirement = FeatureRequirement::Optional;
};

struct TerrainQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
    float lodBias = 1.0f;
    uint32_t clipmapLevels = 8;
};

struct FoliageQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
    float densityScale = 1.0f;
    float lodBias = 0.0f;
};

struct AtmosphereQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
};

struct VolumetricQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    float resolutionScale = 0.5f;
    uint32_t maxSteps = 128;
    uint32_t lightSteps = 6;
    uint32_t shadowSteps = 4;
    uint32_t temporalQuality = 1; // 0 off, 1 basic
    float temporalBlend = 0.85f;
};

// Guard against stale TUs after field growth (ODR/layout mismatch → AV in Resolve).
static_assert(sizeof(VolumetricQualitySettings) == 28,
    "VolumetricQualitySettings size drift — rebuild ALL Renderer objs that include RenderingSettings.h");

struct WaterQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    bool reflections = true;
    bool caustics = false;
};

struct ParticleQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    float budgetScale = 1.0f;
};

struct PostProcessQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::High;
    bool bloom = true;
    bool motionBlur = true;
    bool ambientOcclusion = true;
    bool temporalAA = true;
};

struct LODQualitySettings {
    float globalBias = 0.0f;
    float cullDistanceScale = 1.0f;
};

struct StreamingQualitySettings {
    uint32_t texturePoolMb = 512;
    uint32_t meshPoolMb = 256;
    float ioBudgetScale = 1.0f;
};

/// Explicit capability outcome after resolve — never silently pretend a feature is on.
struct FeatureCapabilityState {
    bool meshShadersAvailable = false;
    bool meshShadersActive = false;
    bool rayTracingAvailable = false;
    bool rayTracingActive = false;
    bool asyncComputeAvailable = false;
    bool asyncComputeActive = false;
    bool multiDrawIndirectAvailable = false;
    bool multiDrawIndirectActive = false;
};

/// Final answer: what rendering configuration should the renderer use right now?
struct ResolvedRenderingSettings {
    RenderingProfileId profileId = RenderingProfileId::HighEnd;
    std::string profileName = "High-End";

    ResolutionSettings resolution;
    GeometryQualitySettings geometry;
    TextureQualitySettings textures;

    LightingQualitySettings lighting;
    ShadowQualitySettings shadows;
    ReflectionQualitySettings reflections;
    GIQualitySettings globalIllumination;

    TerrainQualitySettings terrain;
    FoliageQualitySettings foliage;

    AtmosphereQualitySettings atmosphere;
    VolumetricQualitySettings volumetrics;
    WaterQualitySettings water;

    ParticleQualitySettings particles;
    PostProcessQualitySettings postProcess;

    LODQualitySettings lod;
    StreamingQualitySettings streaming;

    FeatureCapabilityState capabilities;
    std::vector<std::string> fallbackNotes;
};

/// Raw profile data before capability resolution (matches JSON schema).
struct RenderingProfileDesc {
    RenderingProfileId id = RenderingProfileId::HighEnd;
    std::string name = "High-End";
    ResolvedRenderingSettings settings;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
