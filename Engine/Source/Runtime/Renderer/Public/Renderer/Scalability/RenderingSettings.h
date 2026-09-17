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
    uint32_t maxSteps = 32;
};

struct CloudQualitySettings {
    bool enabled = true;
    QualityLevel quality = QualityLevel::Medium;
    uint32_t maxSteps = 32;
    float resolutionScale = 1.0f;
    bool temporalReprojection = true;
};

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
    CloudQualitySettings clouds;
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
