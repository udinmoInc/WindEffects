#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"

#include <cstdint>
#include <vector>

namespace we::runtime::ecs {

// Frame-local render packet. Pure POD / STL — no World, Entity, or Chunk types.
// Renderer consumes this packet without accessing ECS storage.

struct ExtractedMeshDraw {
    std::uint64_t entityId = 0;
    float worldMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    std::uint64_t meshAssetId = 0;
    std::uint64_t materialAssetId = 0;
    float color[4] = {1,1,1,1};
    bool castShadows = true;
    bool receiveShadows = true;
};

struct ExtractedDirectionalLight {
    std::uint64_t entityId = 0;
    float direction[3] = {0, -1, 0};
    float color[3] = {1, 1, 1};
    float intensity = 1.0f;
    bool castShadows = true;
};

struct ExtractedPointLight {
    std::uint64_t entityId = 0;
    float position[3] = {0, 0, 0};
    float color[3] = {1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
};

struct ExtractedSpotLight {
    std::uint64_t entityId = 0;
    float position[3] = {0, 0, 0};
    float direction[3] = {0, -1, 0};
    float color[3] = {1, 1, 1};
    float intensity = 1.0f;
    float range = 10.0f;
    float innerConeDegrees = 20.0f;
    float outerConeDegrees = 30.0f;
};

struct ExtractedCamera {
    std::uint64_t entityId = 0;
    float worldMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    float fieldOfViewDegrees = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;
    bool primary = false;
};

struct ExtractedSkyAtmosphere {
    std::uint64_t entityId = 0;
    bool enabled = true;
    float mieScale = 1.0f;
    float rayleighScale = 1.0f;
};

struct ExtractedVolumetricCloud {
    std::uint64_t entityId = 0;
    bool enabled = true;
    float coverage = 0.55f;
    float density = 1.15f;
    float bottomAltitude = 900.0f;
    float topAltitude = 1600.0f;
};

struct ExtractedTerrain {
    std::uint64_t entityId = 0;
    bool enabled = true;
    std::uint64_t landscapeId = 0;
    int resolutionX = 1009;
    int resolutionY = 1009;
    float worldMatrix[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
};

struct ExtractedWater {
    std::uint64_t entityId = 0;
    bool enabled = true;
    float level = 0.0f;
};

struct ExtractedFrameData {
    std::uint64_t frameIndex = 0;
    std::vector<ExtractedMeshDraw> meshes;
    std::vector<ExtractedDirectionalLight> directionalLights;
    std::vector<ExtractedPointLight> pointLights;
    std::vector<ExtractedSpotLight> spotLights;
    std::vector<ExtractedCamera> cameras;
    std::vector<ExtractedSkyAtmosphere> skyAtmospheres;
    std::vector<ExtractedVolumetricCloud> volumetricClouds;
    std::vector<ExtractedTerrain> terrains;
    std::vector<ExtractedWater> waters;

    void Clear() {
        meshes.clear();
        directionalLights.clear();
        pointLights.clear();
        spotLights.clear();
        cameras.clear();
        skyAtmospheres.clear();
        volumetricClouds.clear();
        terrains.clear();
        waters.clear();
    }

    [[nodiscard]] std::size_t TotalDrawableCount() const {
        return meshes.size() + terrains.size() + waters.size();
    }
};

class World;

// Query visible ECS entities and build frame-local render data.
ECS_API void ExtractRenderFrame(const World& world, ExtractedFrameData& out);

} // namespace we::runtime::ecs

#pragma warning(pop)
