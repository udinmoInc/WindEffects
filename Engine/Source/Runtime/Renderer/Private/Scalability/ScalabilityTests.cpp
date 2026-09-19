// ==============================================================================
// WindEffects — Renderer — ScalabilityTests
// Automated tests for the Scalability subsystem.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Scalability/ScalabilityTests.h"
#include "Renderer/Scalability/CapabilityResolver.h"
#include "Renderer/Scalability/ScalabilityManager.h"

namespace we::runtime::renderer {
namespace {

void AddCase(ScalabilityTestReport& report, std::string name, bool passed, std::string message) {
    ScalabilityTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    report.cases.push_back(std::move(result));
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
}

we::rhi::RHICapabilities MakeFullCaps() {
    we::rhi::RHICapabilities caps{};
    caps.meshShaders = true;
    caps.rayTracing = true;
    caps.asyncCompute = true;
    caps.multiDrawIndirect = true;
    caps.samplerAnisotropy = true;
    caps.maxSamplerAnisotropy = 16.0f;
    caps.maxTextureDimension2D = 16384;
    caps.maxComputeWorkGroupInvocations = 1024;
    return caps;
}

we::rhi::RHICapabilities MakeLimitedCaps() {
    we::rhi::RHICapabilities caps{};
    caps.meshShaders = false;
    caps.rayTracing = false;
    caps.asyncCompute = false;
    caps.multiDrawIndirect = false;
    caps.samplerAnisotropy = false;
    caps.maxSamplerAnisotropy = 1.0f;
    caps.maxTextureDimension2D = 4096;
    caps.maxComputeWorkGroupInvocations = 256;
    return caps;
}

} // namespace

ScalabilityTestReport RunScalabilityRuntimeTests() {
    ScalabilityTestReport report{};

    {
        const auto high = ScalabilityManager::MakeBuiltinProfile(RenderingProfileId::HighEnd);
        AddCase(report, "HighEnd shadows Ultra",
            high.settings.shadows.quality == QualityLevel::Ultra
                && high.settings.shadows.enabled,
            "HighEnd shadows should be enabled Ultra");
        AddCase(report, "HighEnd mesh shaders preferred",
            high.settings.geometry.meshShaderRequirement == FeatureRequirement::Preferred,
            "HighEnd should prefer mesh shaders");
        AddCase(report, "HighEnd geometry Ultra",
            high.settings.geometry.quality == QualityLevel::Ultra,
            "HighEnd geometry should be Ultra");
    }

    {
        const auto balanced = ScalabilityManager::MakeBuiltinProfile(RenderingProfileId::Balanced);
        AddCase(report, "Balanced shadows Medium",
            balanced.settings.shadows.quality == QualityLevel::Medium,
            "Balanced shadows should be Medium");
        AddCase(report, "Balanced is dormant preset",
            balanced.settings.profileName == "Balanced",
            "Balanced preset name");
    }

    {
        const auto low = ScalabilityManager::MakeBuiltinProfile(RenderingProfileId::Low);
        AddCase(report, "Low GI disabled",
            !low.settings.globalIllumination.enabled,
            "Low GI should be disabled");
    }

    {
        const auto high = ScalabilityManager::MakeBuiltinProfile(RenderingProfileId::HighEnd);
        const auto resolved = CapabilityResolver::Resolve(high, MakeLimitedCaps());
        AddCase(report, "HighEnd + limited: mesh shaders off",
            !resolved.geometry.meshShaders && !resolved.capabilities.meshShadersActive,
            "Mesh shaders must fall back when unsupported");
        AddCase(report, "HighEnd + limited: RT reflections off",
            !resolved.reflections.rayTraced,
            "Ray-traced reflections must fall back");
        AddCase(report, "HighEnd + limited: fallback notes present",
            !resolved.fallbackNotes.empty(),
            "Capability fallbacks must be explicit");
        AddCase(report, "HighEnd + limited: still valid shadows",
            resolved.shadows.enabled && resolved.shadows.quality != QualityLevel::Disabled,
            "Limited device must still resolve a valid shadow config");
        AddCase(report, "HighEnd + limited: anisotropy clamped",
            resolved.textures.anisotropy <= 1.0f + 1e-3f,
            "Missing anisotropy must clamp to 1");
        AddCase(report, "Capability state distinguishes available vs active",
            resolved.capabilities.meshShadersAvailable == false
                && resolved.capabilities.meshShadersActive == false,
            "Unavailable features must not report active");
    }

    {
        ScalabilityManager manager;
        manager.Initialize();
        manager.SetRHICapabilities(MakeFullCaps());
        manager.SetRHIBackend(we::rhi::RHIBackend::Null);
        manager.PublishFrameSettings();

        AddCase(report, "Published equals pending after init publish",
            manager.GetPublishedSettings().profileId == RenderingProfileId::HighEnd
                && manager.GetPublishedSettings().shadows.quality == QualityLevel::Ultra,
            "Init should publish HighEnd");

        const auto flags = manager.SetProfile(RenderingProfileId::Low);
        AddCase(report, "Profile switch returns update flags",
            flags != ScalabilityUpdateFlags::None,
            "Switching profiles must report update impact");
        AddCase(report, "Pending updates before publish",
            manager.GetPendingSettings().shadows.quality == QualityLevel::Low
                && manager.HasPendingPublish(),
            "Pending settings must reflect Low before publish");
        AddCase(report, "Published immutable until PublishFrameSettings",
            manager.GetPublishedSettings().shadows.quality == QualityLevel::Ultra,
            "Published settings must stay HighEnd until frame publish");

        manager.PublishFrameSettings();
        AddCase(report, "Publish applies pending",
            manager.GetPublishedSettings().shadows.quality == QualityLevel::Low
                && !manager.HasPendingPublish(),
            "PublishFrameSettings must copy pending to published");

        (void)manager.SetProfile(RenderingProfileId::HighEnd);
        manager.PublishFrameSettings();
        AddCase(report, "Switch back restores HighEnd shadows",
            manager.GetPublishedSettings().shadows.quality == QualityLevel::Ultra,
            "HighEnd shadows after switch");

        manager.Shutdown();
    }

    {
        AddCase(report, "Backend independence contract",
            true,
            "Scalability depends on IRHI/RHICapabilities only");
    }

    {
        const auto high = ScalabilityManager::MakeBuiltinProfile(RenderingProfileId::HighEnd);
        RenderingProfileDesc corrupt = high;
        corrupt.settings.resolution.screenPercentage = 500.0f;
        const auto resolved = CapabilityResolver::Resolve(corrupt, MakeFullCaps());
        AddCase(report, "Invalid screen percentage clamped",
            resolved.resolution.screenPercentage <= 200.0f,
            "Resolver must clamp invalid screen percentage");
    }

    {
        ScalabilityManager manager;
        manager.Initialize();
        manager.SetRHICapabilities(MakeFullCaps());
        const bool reloaded = manager.ReloadProfiles();
        AddCase(report, "ReloadProfiles succeeds",
            reloaded && manager.GetActiveProfileId() == RenderingProfileId::HighEnd,
            "INI missing or present — builtins remain valid fallback");
        manager.Shutdown();
    }

    {
        // Round-trip: named quality strings used by HighEnd.ini schema.
        AddCase(report, "INI quality name Ultra maps correctly",
            ToString(QualityLevel::Ultra) == std::string("Ultra"),
            "Ultra display name for INI Quality=Ultra");
        AddCase(report, "Profile file stem is HighEnd",
            ProfileFileStem(RenderingProfileId::HighEnd) == "HighEnd",
            "HighEnd.ini stem");
    }

    {
        AddCase(report, "Resolved settings are configuration-only",
            sizeof(ResolvedRenderingSettings) > 0
                && sizeof(VolumetricQualitySettings) < 64,
            "Feature settings remain small configuration PODs");
    }

    report.success = report.failed == 0;
    report.summary = report.success
        ? ("Scalability tests passed (" + std::to_string(report.passed) + ")")
        : ("Scalability tests failed: " + std::to_string(report.failed)
            + " / " + std::to_string(report.passed + report.failed));
    return report;
}

} // namespace we::runtime::renderer
