// ==============================================================================
// WindEffects — Renderer — LightingTests
// Automated tests for the Lighting subsystem foundation.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Lighting/LightingTests.h"
#include "Lighting/LightingSystem.h"
#include "Lighting/ShadowSystem.h"
#include "ECS/RenderExtract.h"

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

ScalabilityTestReport RunLightingRuntimeTests() {
    ScalabilityTestReport report{};

    {
        ShadowSystem shadows;
        ShadowQualitySettings settings{};
        settings.enabled = true;
        settings.cascadeCount = 4;
        settings.quality = QualityLevel::Ultra;
        shadows.Configure(settings);

        DirectionalLight sun{};
        sun.castsShadows = true;
        CameraUniform camera{};
        shadows.BeginFrame(camera, &sun);

        AddCase(report, "Shadow cascades configured",
            shadows.Enabled() && shadows.CascadeCount() == 4,
            "Expected 4 enabled cascades");
        AddCase(report, "Shadow splits monotonic",
            shadows.CascadeSplits()[0] > 0.0f
                && shadows.CascadeSplits()[0] < shadows.CascadeSplits()[1]
                && shadows.CascadeSplits()[1] < shadows.CascadeSplits()[2]
                && shadows.CascadeSplits()[2] < shadows.CascadeSplits()[3],
            "Cascade splits should increase");
    }

    {
        ShadowSystem shadows;
        ShadowQualitySettings settings{};
        settings.enabled = false;
        settings.cascadeCount = 4;
        shadows.Configure(settings);
        DirectionalLight sun{};
        CameraUniform camera{};
        shadows.BeginFrame(camera, &sun);
        AddCase(report, "Shadows disabled by settings",
            !shadows.Enabled() && shadows.CascadeCount() == 0,
            "Disabled shadows should report zero cascades");
    }

    {
        LightingSystem lighting;
        AddCase(report, "LightingSystem initialize without device",
            lighting.Initialize(LightingCreateInfo{}),
            "CPU-side init should succeed");

        LightingQualitySettings lightingSettings{};
        lightingSettings.enabled = true;
        lightingSettings.maxLocalLights = 8;
        ShadowQualitySettings shadowSettings{};
        shadowSettings.enabled = true;
        shadowSettings.cascadeCount = 3;
        lighting.Configure(lightingSettings, shadowSettings);

        we::runtime::ecs::ExtractedFrameData extract{};
        we::runtime::ecs::ExtractedDirectionalLight dir{};
        dir.direction[0] = 0.0f;
        dir.direction[1] = -1.0f;
        dir.direction[2] = 0.0f;
        dir.color[0] = 1.0f;
        dir.color[1] = 0.9f;
        dir.color[2] = 0.8f;
        dir.intensity = 2.5f;
        dir.castShadows = true;
        extract.directionalLights.push_back(dir);

        for (int i = 0; i < 12; ++i) {
            we::runtime::ecs::ExtractedPointLight point{};
            point.position[0] = static_cast<float>(i);
            point.intensity = 1.0f;
            point.range = 5.0f;
            extract.pointLights.push_back(point);
        }

        SceneEnvironmentUniform environment{};
        environment.sunIntensity = 0.1f;
        CameraUniform camera{};

        LightingFrameContext ctx{};
        ctx.extract = &extract;
        ctx.camera = &camera;
        ctx.environment = &environment;
        lighting.BeginFrame(ctx);

        AddCase(report, "Directional from extract",
            lighting.DirectionalLightCount() == 1
                && lighting.GetScene().directionalLights[0].intensity == 2.5f,
            "Primary directional should come from extract");
        AddCase(report, "Local light budget clamp",
            lighting.PointLightCount() == 8,
            "Point lights should clamp to maxLocalLights");
        AddCase(report, "Environment sun sync",
            environment.sunIntensity == 2.5f
                && environment.sunDirection.y < 0.0f
                && environment.sunCastShadows == 1,
            "Primary light should write back into environment UBO fields");
        AddCase(report, "Shadow system follows lighting",
            lighting.GetShadows().Enabled() && lighting.GetShadows().CascadeCount() == 3,
            "Shadows should activate for casting directional");

        lighting.Shutdown();
    }

    {
        LightingSystem lighting;
        lighting.Initialize(LightingCreateInfo{});
        LightingQualitySettings lightingSettings{};
        lightingSettings.enabled = true;
        lighting.Configure(lightingSettings, ShadowQualitySettings{});

        SceneEnvironmentUniform environment{};
        environment.sunDirection = {0.2f, -0.9f, 0.1f};
        environment.sunColor = {1.0f, 1.0f, 1.0f};
        environment.sunIntensity = 1.4f;
        environment.sunCastShadows = 1;

        LightingFrameContext ctx{};
        ctx.environment = &environment;
        lighting.BeginFrame(ctx);

        AddCase(report, "Fallback sun from environment",
            lighting.DirectionalLightCount() == 1
                && lighting.GetScene().directionalLights[0].intensity == 1.4f,
            "Without extract lights, environment sun should seed LightingScene");
        lighting.Shutdown();
    }

    {
        LightingSystem lighting;
        lighting.Initialize(LightingCreateInfo{});
        LightingQualitySettings lightingSettings{};
        lightingSettings.enabled = false;
        lighting.Configure(lightingSettings, ShadowQualitySettings{});

        we::runtime::ecs::ExtractedFrameData extract{};
        we::runtime::ecs::ExtractedDirectionalLight dir{};
        extract.directionalLights.push_back(dir);
        LightingFrameContext ctx{};
        ctx.extract = &extract;
        lighting.BeginFrame(ctx);

        AddCase(report, "Lighting disabled clears scene",
            lighting.DirectionalLightCount() == 0 && !lighting.Enabled(),
            "Disabled lighting should produce an empty GPU light list");
        lighting.Shutdown();
    }

    report.success = report.failed == 0;
    report.summary = "Lighting tests: "
        + std::to_string(report.passed) + " passed, "
        + std::to_string(report.failed) + " failed";
    return report;
}

} // namespace we::runtime::renderer
