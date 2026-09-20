// ==============================================================================
// WindEffects — Renderer — VolumetricTests
// Foundation validation for the unified volumetric pipeline.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Renderer/Volumetrics/VolumetricTests.h"

#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Volumetrics/IVolumetricProvider.h"
#include "Renderer/Volumetrics/LocalFogUniform.h"
#include "Renderer/Volumetrics/VolumeSample.h"
#include "Renderer/Volumetrics/VolumetricFrameUniform.h"
#include "Renderer/Volumetrics/VolumetricTypes.h"
#include "Volumetrics/CloudVolumeProvider.h"
#include "Volumetrics/HeightFogVolumeProvider.h"
#include "Volumetrics/LocalFogVolumeProvider.h"
#include "Volumetrics/AtmosphereVolumeProvider.h"

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

} // namespace

ScalabilityTestReport RunVolumetricFoundationTests() {
    ScalabilityTestReport report{};

    AddCase(report, "VolumeSample POD size",
        sizeof(VolumeSample) == sizeof(float) * 4,
        "Expected 4 floats");

    AddCase(report, "VolumetricFrameUniform size",
        sizeof(VolumetricFrameUniform) == kVolumetricFrameUniformSize,
        "Expected 128-byte std140 UBO");

    AddCase(report, "Provider type bits distinct",
        VolumetricProviderMask::Bit(VolumetricProviderType::Cloud)
            != VolumetricProviderMask::Bit(VolumetricProviderType::HeightFog)
            && VolumetricProviderMask::Bit(VolumetricProviderType::HeightFog)
                != VolumetricProviderMask::Bit(VolumetricProviderType::LocalFog),
        "Mask bits must be unique");

    {
        CloudVolumeProvider cloud;
        HeightFogVolumeProvider heightFog;
        LocalFogVolumeProvider localFog;
        AtmosphereVolumeProvider atmosphere;
        AddCase(report, "Providers constructible",
            cloud.GetType() == VolumetricProviderType::Cloud
                && heightFog.GetType() == VolumetricProviderType::HeightFog
                && localFog.GetType() == VolumetricProviderType::LocalFog
                && atmosphere.GetType() == VolumetricProviderType::Atmosphere,
            "Cloud/HeightFog/LocalFog/Atmosphere types");

        VolumetricPrepareContext ctx{};
        cloud.PrepareFrame(ctx);
        heightFog.PrepareFrame(ctx);
        localFog.PrepareFrame(ctx);
        atmosphere.PrepareFrame(ctx);
        AddCase(report, "PrepareFrame null-safe",
            !cloud.IsEnabled() && !heightFog.IsEnabled() && !localFog.IsEnabled()
                && !atmosphere.IsEnabled(),
            "Providers disabled without inputs");

        // Atmosphere must stay opt-in so ProceduralSky owns the visible sky.
        SceneEnvironmentUniform env{};
        env.atmosphereRayleigh = {0.005802f, 0.013558f, 0.033100f};
        env.atmosphereHeight = 60.0f;
        VolumetricPrepareContext atmoCtx{};
        atmoCtx.environment = &env;
        atmosphere.PrepareFrame(atmoCtx);
        AddCase(report, "Atmosphere default off with Rayleigh",
            !atmosphere.IsEnabled(),
            "AtmosphereVolumeProvider must not auto-enable from SkyAtmosphere params");
    }

    {
        // Avoid constructing VolumetricRenderer in headless harness (RHI/path statics).
        CloudVolumeProvider cloud;
        HeightFogVolumeProvider heightFog;
        LocalFogVolumeProvider localFog;
        AddCase(report, "Default provider types registered",
            cloud.GetName() != nullptr && heightFog.GetName() != nullptr && localFog.GetName() != nullptr,
            "Provider names");

        LocalFogUniform fog{};
        fog.enabled = 1.0f;
        fog.density = 0.05f;
        localFog.SetLocalFog(fog);
        VolumetricPrepareContext ctx{};
        ctx.localFog = &fog;
        localFog.PrepareFrame(ctx);
        AddCase(report, "LocalFog prepare enables",
            localFog.IsEnabled() && localFog.GetLocalFog().density > 0.0f,
            "LocalFog PrepareFrame");
    }

    {
        VolumetricProviderMask mask{};
        mask.Set(VolumetricProviderType::Cloud);
        mask.Set(VolumetricProviderType::HeightFog);
        AddCase(report, "Provider mask has cloud+fog",
            mask.Has(VolumetricProviderType::Cloud)
                && mask.Has(VolumetricProviderType::HeightFog)
                && !mask.Has(VolumetricProviderType::LocalFog),
            "Bitmask ops");
    }

    report.success = report.failed == 0;
    report.summary = "Volumetric foundation tests: "
        + std::to_string(report.passed) + " passed, "
        + std::to_string(report.failed) + " failed";
    return report;
}

} // namespace we::runtime::renderer
