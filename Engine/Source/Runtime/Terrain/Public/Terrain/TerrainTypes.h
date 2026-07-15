#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "Terrain/Export.h"

#include <cstdint>
#include <string>
#include <vector>

#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::terrain {

// Landscape-style defaults (UE-inspired): 127 quads → 128 verts per axis per chunk.
inline constexpr int kDefaultChunkQuads = 127;
inline constexpr int kDefaultChunkVerts = kDefaultChunkQuads + 1; // 128
inline constexpr int kMaxLodLevels = 5;
inline constexpr int kMaxPaintLayers = 4; // packed into RGBA weight maps

enum class TerrainBrushOp {
    Raise,
    Lower,
    Smooth,
    Flatten,
    Noise,
    HydraulicErosion,
    ThermalErosion
};

enum class HeightmapFormat {
    Png8,
    Png16,
    Raw16Le, // little-endian uint16 dump
    R16      // same as Raw16Le, distinct extension
};

struct TerrainCreateInfo {
    int resolutionX = 1009; // (7 * 127) + 1 → 8x8 chunks of 127 quads
    int resolutionY = 1009;
    float worldSizeX = 1009.0f; // meters
    float worldSizeY = 1009.0f;
    float heightScale = 256.0f; // meters at max uint16
    float heightOffset = 0.0f;
    int chunkQuads = kDefaultChunkQuads;
    glm::vec3 worldOrigin{ 0.0f, 0.0f, 0.0f };
};

struct TerrainChunkId {
    int x = 0;
    int z = 0;

    bool operator==(const TerrainChunkId& o) const { return x == o.x && z == o.z; }
};

struct TerrainChunkIdHash {
    std::size_t operator()(const TerrainChunkId& id) const {
        return (static_cast<std::size_t>(id.x) * 73856093u)
            ^ (static_cast<std::size_t>(id.z) * 19349663u);
    }
};

struct TerrainAABB {
    glm::vec3 min{ 0.0f };
    glm::vec3 max{ 0.0f };

    bool IntersectsFrustumPlanes(const glm::vec4 planes[6]) const;
    bool ContainsPoint(const glm::vec3& p) const;
};

struct TerrainFrustump {
    glm::vec4 planes[6]{};
    static TerrainFrustump FromViewProj(const glm::mat4& viewProj);
};

struct TerrainMeshCPU {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<std::uint32_t> indices;
};

struct FoliageInstance {
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
    float yawDegrees = 0.0f;
    float scale = 1.0f;
    int layerIndex = 0;
};

struct FoliageSpawnParams {
    int layerIndex = 0;
    float densityPerSquareMeter = 0.05f;
    float minSlopeDegrees = 0.0f;
    float maxSlopeDegrees = 35.0f;
    float minHeight = -1e6f;
    float maxHeight = 1e6f;
    float minScale = 0.8f;
    float maxScale = 1.2f;
    std::uint32_t seed = 1u;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
