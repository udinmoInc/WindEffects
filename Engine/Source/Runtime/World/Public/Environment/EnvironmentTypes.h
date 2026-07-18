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
