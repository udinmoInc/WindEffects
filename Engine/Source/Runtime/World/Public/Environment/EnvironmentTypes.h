// ==============================================================================
// WindEffects — World — EnvironmentTypes
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>

namespace we::runtime::world::environment {

constexpr const char* kEnvironmentFolderName = "Environment";
constexpr const char* kEnvironmentManagerActorName = "Environment Manager";
constexpr const char* kSunActorName = "Sun";
constexpr const char* kSkyLightActorName = "SkyLight";
constexpr const char* kSkyAtmosphereActorName = "SkyAtmosphere";
constexpr const char* kHeightFogActorName = "ExponentialHeightFog";
constexpr const char* kVolumetricCloudsActorName = "VolumetricClouds";
constexpr const char* kExposureControllerActorName = "Exposure Controller";

enum class EnvironmentPreset {
    Sunny,
    Sunset,
    Night,
    Overcast,
    Foggy,
    Studio
};

enum class EnvironmentActorKind {
    Folder,
    EnvironmentManager,
    DirectionalLight,
    SkyLight,
    SkyAtmosphere,
    HeightFog,
    VolumetricClouds,
    ExposureController
};

} // namespace we::runtime::world::environment
