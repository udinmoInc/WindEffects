// ==============================================================================
// WindEffects — Renderer — CapabilityResolver
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/CapabilityResolver.h"

#include <algorithm>
#include <string>

namespace we::runtime::renderer {
namespace {

bool ResolveFeatureGate(
    FeatureRequirement requirement,
    bool available,
    const char* featureName,
    std::vector<std::string>& notes) {
    switch (requirement) {
    case FeatureRequirement::Off:
        return false;
    case FeatureRequirement::Optional:
    case FeatureRequirement::Preferred:
        if (!available) {
            notes.emplace_back(std::string(featureName) + " unavailable — using fallback path");
            return false;
        }
        return true;
    case FeatureRequirement::Required:
        if (!available) {
            notes.emplace_back(std::string(featureName)
                + " required by profile but unsupported — feature path disabled");
            return false;
        }
        return true;
    }
    return false;
}

QualityLevel CapByAnisotropy(QualityLevel level, float maxAniso) {
    if (maxAniso < 2.0f) {
        return ClampQuality(level, QualityLevel::Low);
    }
    if (maxAniso < 8.0f) {
        return ClampQuality(level, QualityLevel::Medium);
    }
    return level;
}

} // namespace

ResolvedRenderingSettings CapabilityResolver::Resolve(
    const RenderingProfileDesc& profile,
    const we::rhi::RHICapabilities& caps) {
    ResolvedRenderingSettings out = profile.settings;
    out.profileId = profile.id;
    out.profileName = profile.name;
    out.fallbackNotes.clear();

    out.capabilities.meshShadersAvailable = caps.meshShaders;
    out.capabilities.rayTracingAvailable = caps.rayTracing;
    out.capabilities.asyncComputeAvailable = caps.asyncCompute;
    out.capabilities.multiDrawIndirectAvailable = caps.multiDrawIndirect;

    out.capabilities.meshShadersActive = ResolveFeatureGate(
        out.geometry.meshShaderRequirement,
        caps.meshShaders,
        "MeshShaders",
        out.fallbackNotes);
    out.geometry.meshShaders = out.capabilities.meshShadersActive;

    const bool rtReflections = ResolveFeatureGate(
        out.reflections.rayTracingRequirement,
        caps.rayTracing,
        "RayTracedReflections",
        out.fallbackNotes);
    out.reflections.rayTraced = out.reflections.rayTraced && rtReflections;
    if (!out.reflections.rayTraced && out.reflections.enabled && !out.reflections.screenSpace) {
        out.reflections.screenSpace = true;
        out.fallbackNotes.emplace_back("Reflections fell back to screen-space path");
    }

    const bool rtGi = ResolveFeatureGate(
        out.globalIllumination.rayTracingRequirement,
        caps.rayTracing,
        "RayTracedGI",
        out.fallbackNotes);
    out.globalIllumination.rayTraced = out.globalIllumination.rayTraced && rtGi;
    out.capabilities.rayTracingActive = out.reflections.rayTraced || out.globalIllumination.rayTraced;

    out.capabilities.asyncComputeActive = caps.asyncCompute;
    out.capabilities.multiDrawIndirectActive = caps.multiDrawIndirect;

    if (out.resolution.msaaSamples > 1) {
        // RHICapabilities has no explicit max MSAA yet — clamp to a safe desktop default.
        out.resolution.msaaSamples = std::min(out.resolution.msaaSamples, 8u);
    }

    out.resolution.maxShadowMapResolution = std::min(
        out.resolution.maxShadowMapResolution,
        caps.maxTextureDimension2D);

    if (!caps.samplerAnisotropy) {
        out.textures.anisotropy = 1.0f;
        out.fallbackNotes.emplace_back("Anisotropic filtering unsupported — anisotropy forced to 1");
    } else {
        out.textures.anisotropy = std::min(out.textures.anisotropy, caps.maxSamplerAnisotropy);
        out.textures.quality = CapByAnisotropy(out.textures.quality, caps.maxSamplerAnisotropy);
    }

    if (!out.shadows.enabled) {
        out.shadows.quality = QualityLevel::Disabled;
        out.shadows.cascadeCount = 0;
    }
    if (!out.clouds.enabled) {
        out.clouds.quality = QualityLevel::Disabled;
        out.clouds.maxSteps = 0;
        out.clouds.temporalReprojection = false;
    }
    if (!out.volumetrics.enabled) {
        out.volumetrics.quality = QualityLevel::Disabled;
        out.volumetrics.maxSteps = 0;
    }
    if (!out.reflections.enabled) {
        out.reflections.quality = QualityLevel::Disabled;
        out.reflections.screenSpace = false;
        out.reflections.rayTraced = false;
    }
    if (!out.globalIllumination.enabled) {
        out.globalIllumination.quality = QualityLevel::Disabled;
        out.globalIllumination.rayTraced = false;
    }

    out.resolution.screenPercentage = std::clamp(out.resolution.screenPercentage, 25.0f, 200.0f);
    return out;
}

} // namespace we::runtime::renderer
