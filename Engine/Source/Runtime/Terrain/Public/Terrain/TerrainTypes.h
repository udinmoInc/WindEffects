#pragma once

#include "Terrain/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::terrain {

/// Landscape-style defaults: 127 quads → 128 verts per axis per chunk.
inline constexpr int kDefaultChunkQuads = 127;
inline constexpr int kDefaultChunkVerts = kDefaultChunkQuads + 1; // 128
inline constexpr int kMaxLodLevels = 5;
inline constexpr int kMaxPaintLayers = 4; // packed into RGBA weight maps
inline constexpr int kDefaultTileSize = 1;
inline constexpr float kMidHeightNormalized = 0.5f;
inline constexpr std::uint16_t kMidHeightSample = 32768;

/// Built-in engine landscape material (Engine Content — never project Content).
inline constexpr const char* kDefaultLandscapeMaterialPath =
    "Engine/Content/Materials/M_DefaultLandscape.wemat";
inline constexpr const char* kDefaultLandscapeMaterialRelative =
    "Materials/M_DefaultLandscape.wemat";

/// Matte charcoal (UE5-editor style) — replaces legacy green debug albedo.
inline constexpr float kDefaultLandscapeAlbedoR = 0.165f;
inline constexpr float kDefaultLandscapeAlbedoG = 0.165f;
inline constexpr float kDefaultLandscapeAlbedoB = 0.175f;
inline constexpr float kDefaultLandscapeRoughness = 0.92f;
inline constexpr float kDefaultLandscapeMetallic = 0.0f;

/// Stable terrain asset identity (maps to AssetGuid / WorldGuid bytes).
struct TERRAIN_API TerrainGuid {
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return hi != 0 || lo != 0; }
    [[nodiscard]] constexpr bool operator==(const TerrainGuid& o) const noexcept {
        return hi == o.hi && lo == o.lo;
    }
    [[nodiscard]] constexpr bool operator!=(const TerrainGuid& o) const noexcept {
        return !(*this == o);
    }
};

struct TERRAIN_API TerrainGuidHash {
    [[nodiscard]] std::size_t operator()(const TerrainGuid& g) const noexcept {
        return static_cast<std::size_t>(g.hi ^ (g.lo * 0x9e3779b97f4a7c15ull));
    }
};

struct TERRAIN_API TerrainId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const TerrainId& o) const noexcept {
        return value == o.value;
    }
};

struct TERRAIN_API TerrainIdHash {
    [[nodiscard]] std::size_t operator()(const TerrainId& id) const noexcept {
        return static_cast<std::size_t>(id.value);
    }
};

/// How a landscape is initially populated. Heightmap file import is optional — never required.
enum class TerrainCreationMethod : std::uint8_t {
    Flat = 0,          // default — editable flat landscape, no heightmap file
    Empty,             // zero elevation samples (still editable)
    Procedural,        // generator-selected procedural fill
    Noise,             // alias for Perlin/FBM family
    Fractal,           // FBM / ridged fractal
    HeightmapImport,   // optional PNG/RAW import
    PluginGenerator,   // plugin-registered ITerrainGenerator
};

enum class TerrainGeneratorId : std::uint8_t {
    Flat = 0,
    Empty,
    PerlinNoise,
    Fbm,
    RidgedNoise,
    Voronoi,
    Island,
    Mountain,
    Plateau,
    RandomSeed,
    Custom = 255,
};

enum class TerrainBrushOp : std::uint8_t {
    Raise = 0,
    Lower,
    Smooth,
    Flatten,
    Noise,
    Paint,
    Ramp,
    Terrace,
    HydraulicErosion,
    ThermalErosion,
    CustomAlpha,
};

enum class HeightmapFormat : std::uint8_t {
    Png8 = 0,
    Png16,
    Raw16Le, // little-endian uint16 dump
    R16,     // same as Raw16Le, distinct extension
};

enum class TerrainEventKind : std::uint8_t {
    Created = 0,
    Destroyed,
    Generated,
    Imported,
    Exported,
    Edited,
    Streamed,
    Validated,
    LodChanged,
    CollisionRebuilt,
};

enum class TerrainValidationSeverity : std::uint8_t {
    Info = 0,
    Warning,
    Error,
};

struct TERRAIN_API TerrainLODSettings {
    int maxLod = 3;
    float lodBias = 1.0f;
    bool enabled = true;
};

struct TERRAIN_API TerrainStreamingOptions {
    bool enabled = true;
    int loadRadiusChunks = 8;
    int maxResidentChunks = 64;
    bool asyncLoad = true;
    bool asyncCollision = true;
};

struct TERRAIN_API TerrainCollisionOptions {
    bool enabled = true;
    bool asyncBuild = true;
    bool rebuildOnEdit = true;
};

struct TERRAIN_API TerrainGeneratorParams {
    TerrainGeneratorId generator = TerrainGeneratorId::Flat;
    std::uint32_t seed = 1u;
    float frequency = 0.008f;
    int octaves = 4;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float amplitude = 0.35f;     // relative to mid-height
    float baseHeight = 0.5f;     // normalized 0..1
    float ridgeStrength = 0.4f;
    float islandFalloff = 0.85f;
    float plateauLevel = 0.65f;
    float mountainPeak = 0.9f;
    std::string pluginGeneratorId; // when generator == Custom
};

/// Backward-compatible alias used by older procedural call sites.
using ProceduralHeightmapParams = TerrainGeneratorParams;

/// Full New Landscape / Create Landscape description. Heightmap path is optional.
struct TERRAIN_API TerrainCreateInfo {
    int resolutionX = 1009; // (7 * 127) + 1 → 8x8 chunks of 127 quads
    int resolutionY = 1009;
    float worldSizeX = 1009.0f; // meters (width)
    float worldSizeY = 1009.0f; // meters (depth / "height" in plan)
    float heightScale = 256.0f; // meters at max uint16
    float heightOffset = 0.0f;
    float initialElevation = 0.5f; // normalized fill for Flat
    int chunkQuads = kDefaultChunkQuads;
    int tileSize = kDefaultTileSize;
    we::math::Vec3 worldOrigin{0.0f, 0.0f, 0.0f};
    we::math::Vec3 worldScale{1.0f, 1.0f, 1.0f};
    TerrainCreationMethod creationMethod = TerrainCreationMethod::Flat;
    TerrainGeneratorParams generator{};
    TerrainLODSettings lod{};
    TerrainStreamingOptions streaming{};
    TerrainCollisionOptions collision{};
    std::string materialSlot0;
    std::string materialSlot1;
    std::string materialSlot2;
    std::string materialSlot3;
    std::string displayName = "Landscape";
};

struct TERRAIN_API TerrainChunkId {
    int x = 0;
    int z = 0;

    bool operator==(const TerrainChunkId& o) const { return x == o.x && z == o.z; }
};

struct TERRAIN_API TerrainChunkIdHash {
    std::size_t operator()(const TerrainChunkId& id) const {
        return (static_cast<std::size_t>(id.x) * 73856093u)
            ^ (static_cast<std::size_t>(id.z) * 19349663u);
    }
};

struct TERRAIN_API TerrainAABB {
    we::math::Vec3 min{0.0f};
    we::math::Vec3 max{0.0f};

    bool IntersectsFrustumPlanes(const we::math::Vec4 planes[6]) const;
    bool ContainsPoint(const we::math::Vec3& p) const;
};

struct TERRAIN_API TerrainFrustump {
    we::math::Vec4 planes[6]{};
    static TerrainFrustump FromViewProj(const we::math::Mat4& viewProj);
};

struct TERRAIN_API TerrainMeshCPU {
    std::vector<we::math::Vec3> positions;
    std::vector<we::math::Vec3> normals;
    std::vector<we::math::Vec2> uvs;
    std::vector<std::uint32_t> indices;
};

struct TERRAIN_API TerrainRenderStats {
    std::uint32_t chunksCreated = 0;
    std::uint32_t chunksVisible = 0;
    std::uint32_t chunksLoaded = 0;
    std::uint32_t chunksStreamed = 0;
    std::uint32_t chunksRebuilt = 0;
    std::uint32_t chunksUploaded = 0;
    std::uint32_t vertices = 0;
    std::uint32_t triangles = 0;
    std::uint32_t gpuBuffers = 0;
    std::uint32_t drawCalls = 0;
    std::uint64_t gpuUploadMicros = 0;
    std::uint64_t memoryBytes = 0;
    bool renderProxyReady = false;
    bool pipelineReady = false;
    TerrainAABB bounds{};
    std::string materialAssigned = kDefaultLandscapeMaterialPath;
};

struct TERRAIN_API TerrainCreationReport {
    bool success = false;
    std::string failedStage;
    std::string reason;
    TerrainRenderStats render{};
    std::uint64_t entityId = 0;
    TerrainId terrainId{};
};

struct TERRAIN_API TerrainBrushSettings {
    TerrainBrushOp op = TerrainBrushOp::Raise;
    float radius = 16.0f;      // heightfield samples
    float strength = 0.35f;    // 0..1
    float falloff = 0.5f;      // soft edge
    float targetHeight = 0.5f; // normalized flatten target
    float noiseScale = 0.15f;
    float rampHeight = 0.1f;   // normalized ramp delta
    float terraceStep = 0.05f; // normalized terrace spacing
    int erosionIterations = 8;
    int paintLayer = 0;
    /// Optional custom alpha brush (row-major 0..255). Empty = circular falloff.
    std::vector<std::uint8_t> alphaMask;
    int alphaWidth = 0;
    int alphaHeight = 0;
};

struct TERRAIN_API TerrainValidationIssue {
    TerrainValidationSeverity severity = TerrainValidationSeverity::Error;
    std::string message;
    TerrainId terrain{};
    TerrainGuid asset{};
};

struct TERRAIN_API TerrainConfig {
    bool allowAsyncGeneration = true;
    bool allowAsyncCollision = true;
    bool deterministicEditing = true;
    std::uint32_t maxTerrains = 256;
};

struct TERRAIN_API FoliageInstance {
    we::math::Vec3 position{0.0f};
    we::math::Vec3 normal{0.0f, 1.0f, 0.0f};
    float yawDegrees = 0.0f;
    float scale = 1.0f;
    int layerIndex = 0;
};

struct TERRAIN_API FoliageSpawnParams {
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
