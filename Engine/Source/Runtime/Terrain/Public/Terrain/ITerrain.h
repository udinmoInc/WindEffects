#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "RHI/Types.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::terrain {

class ITerrain;
class TerrainHeightmap;

/// World-bound terrain component (actor/entity link). Terrain is a normal World Runtime actor.
class TERRAIN_API ITerrainComponent {
public:
    virtual ~ITerrainComponent() = default;

    [[nodiscard]] virtual TerrainId GetTerrainId() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetEntityId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    virtual void SetWorldTransform(
        const we::math::Vec3& origin,
        const we::math::Vec3& scale) = 0;
};

/// Serializable terrain asset — elevation samples live in-memory; heightmap files are optional.
class TERRAIN_API ITerrainAsset {
public:
    virtual ~ITerrainAsset() = default;

    [[nodiscard]] virtual TerrainGuid GetGuid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetAssetPath() const noexcept = 0;
    [[nodiscard]] virtual const TerrainCreateInfo& GetCreateInfo() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::uint16_t> GetElevationSamples() const noexcept = 0;
    [[nodiscard]] virtual int GetWidth() const noexcept = 0;
    [[nodiscard]] virtual int GetHeight() const noexcept = 0;
    [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    [[nodiscard]] virtual bool IsDirty() const noexcept = 0;
};

/// Single chunk of a landscape (cache-friendly residency unit).
class TERRAIN_API ITerrainChunk {
public:
    virtual ~ITerrainChunk() = default;

    [[nodiscard]] virtual TerrainChunkId GetId() const noexcept = 0;
    [[nodiscard]] virtual int GetLod() const noexcept = 0;
    [[nodiscard]] virtual bool IsVisible() const noexcept = 0;
    [[nodiscard]] virtual bool IsMeshDirty() const noexcept = 0;
    [[nodiscard]] virtual const TerrainAABB& GetBounds() const noexcept = 0;
    [[nodiscard]] virtual const TerrainMeshCPU* GetMesh() const noexcept = 0;
};

/// LOD policy for chunked terrain.
class TERRAIN_API ITerrainLOD {
public:
    virtual ~ITerrainLOD() = default;

    virtual void SetSettings(const TerrainLODSettings& settings) = 0;
    [[nodiscard]] virtual TerrainLODSettings GetSettings() const noexcept = 0;
    virtual void Update(
        const we::math::Vec3& cameraWorldPos,
        const TerrainCreateInfo& info,
        int heightfieldWidth) = 0;
};

/// Paint / material layer slot.
class TERRAIN_API ITerrainLayer {
public:
    virtual ~ITerrainLayer() = default;

    [[nodiscard]] virtual int GetIndex() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    virtual void SetName(std::string_view name) = 0;
    virtual void SetAlbedoPath(std::string_view path) = 0;
    [[nodiscard]] virtual std::string_view GetAlbedoPath() const noexcept = 0;
};

/// Material stack for a terrain (weight maps + layer slots; VT-ready).
class TERRAIN_API ITerrainMaterial {
public:
    virtual ~ITerrainMaterial() = default;

    [[nodiscard]] virtual int GetLayerCount() const noexcept = 0;
    [[nodiscard]] virtual ITerrainLayer* GetLayer(int index) = 0;
    virtual void PaintWeight(int x, int z, int layerIndex, float strength, float radius) = 0;
    virtual void SetVirtualTexturingEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool WantsVirtualTexturing() const noexcept = 0;
};

/// Collision queries / async collider generation hooks.
class TERRAIN_API ITerrainCollision {
public:
    virtual ~ITerrainCollision() = default;

    virtual void SetOptions(const TerrainCollisionOptions& options) = 0;
    [[nodiscard]] virtual TerrainCollisionOptions GetOptions() const noexcept = 0;
    [[nodiscard]] virtual bool Raycast(
        const we::math::Vec3& origin,
        const we::math::Vec3& direction,
        float maxDistance,
        we::math::Vec3& outHit,
        we::math::Vec3& outNormal) const = 0;
    [[nodiscard]] virtual bool SampleHeightNormal(
        float worldX,
        float worldZ,
        float& outHeight,
        we::math::Vec3& outNormal) const = 0;
    virtual void RebuildDirty() = 0;
};

/// Chunk residency / streaming (world-partition ready).
class TERRAIN_API ITerrainStreaming {
public:
    virtual ~ITerrainStreaming() = default;

    virtual void SetOptions(const TerrainStreamingOptions& options) = 0;
    [[nodiscard]] virtual TerrainStreamingOptions GetOptions() const noexcept = 0;
    virtual void Update(
        const we::math::Vec3& cameraWorldPos,
        const TerrainFrustump* frustum) = 0;
    virtual void RequestLoad(TerrainChunkId id) = 0;
    virtual void RequestUnload(TerrainChunkId id) = 0;
};

/// GPU-friendly chunk submission.
class TERRAIN_API ITerrainRenderer {
public:
    virtual ~ITerrainRenderer() = default;

    virtual void BindDevice(rhi::IRHIDevice* device) = 0;
    virtual void SyncChunks() = 0;
    virtual void DrawViewport(
        rhi::IRHICommandList& cmd,
        rhi::RHITextureHandle color,
        rhi::RHITextureHandle depth,
        rhi::Extent2D extent,
        const we::math::Mat4& view,
        const we::math::Mat4& proj,
        const we::math::Vec3& cameraPos,
        const TerrainSceneLighting* lighting = nullptr) = 0;
    virtual void SetClipmapsEnabled(bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t UploadedChunkCount() const noexcept = 0;
    [[nodiscard]] virtual const TerrainRenderStats& LastStats() const noexcept = 0;
    [[nodiscard]] virtual bool IsReady() const noexcept = 0;
    [[nodiscard]] virtual bool HasDevice() const noexcept = 0;
};

/// Sculpt / paint brush service (stateless apply over heightfield).
class TERRAIN_API ITerrainBrush {
public:
    virtual ~ITerrainBrush() = default;

    virtual void SetSettings(const TerrainBrushSettings& settings) = 0;
    [[nodiscard]] virtual const TerrainBrushSettings& GetSettings() const noexcept = 0;
    /// Begin continuous stroke — mesh/GPU dirty updates only; collision deferred.
    virtual void BeginStroke() = 0;
    /// Finalize stroke — rebuild dirty collision (optionally async).
    virtual void EndStroke() = 0;
    [[nodiscard]] virtual bool IsStrokeActive() const noexcept = 0;
    [[nodiscard]] virtual bool ApplyAtSample(float sampleX, float sampleZ) = 0;
    [[nodiscard]] virtual bool ApplyAtWorld(float worldX, float worldZ) = 0;
    virtual void SetCustomAlpha(
        std::span<const std::uint8_t> mask,
        int width,
        int height) = 0;
};

/// Deterministic elevation generator. Plugins register additional generators.
class TERRAIN_API ITerrainGenerator {
public:
    virtual ~ITerrainGenerator() = default;

    [[nodiscard]] virtual std::string_view GetId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual TerrainGeneratorId GetBuiltinId() const noexcept = 0;
    [[nodiscard]] virtual bool Generate(
        TerrainHeightmap& heightfield,
        const TerrainCreateInfo& info,
        const TerrainGeneratorParams& params) const = 0;
};

/// Optional heightmap file importer (PNG / RAW / 16-bit). Never required for landscape creation.
class TERRAIN_API ITerrainImporter {
public:
    virtual ~ITerrainImporter() = default;

    [[nodiscard]] virtual bool ImportHeightmapFile(
        std::string_view path,
        TerrainHeightmap& outHeightfield,
        HeightmapFormat* detected = nullptr) = 0;
    [[nodiscard]] virtual bool ImportRaw16(
        std::string_view path,
        int width,
        int height,
        TerrainHeightmap& outHeightfield,
        bool littleEndian = true) = 0;
};

/// Optional heightmap file exporter.
class TERRAIN_API ITerrainExporter {
public:
    virtual ~ITerrainExporter() = default;

    [[nodiscard]] virtual bool ExportHeightmapFile(
        std::string_view path,
        const TerrainHeightmap& heightfield,
        HeightmapFormat format) = 0;
    [[nodiscard]] virtual bool ExportEditedTerrain(
        std::string_view path,
        const ITerrain& terrain,
        HeightmapFormat format) = 0;
};

class TERRAIN_API ITerrainValidator {
public:
    virtual ~ITerrainValidator() = default;

    [[nodiscard]] virtual std::vector<TerrainValidationIssue> Validate(
        const ITerrain& terrain) const = 0;
    [[nodiscard]] virtual std::vector<TerrainValidationIssue> ValidateCreateInfo(
        const TerrainCreateInfo& info) const = 0;
};

/// Live landscape instance — elevation grid + chunks; independent of heightmap files.
class TERRAIN_API ITerrain {
public:
    virtual ~ITerrain() = default;

    [[nodiscard]] virtual TerrainId GetId() const noexcept = 0;
    [[nodiscard]] virtual TerrainGuid GetAssetGuid() const noexcept = 0;
    [[nodiscard]] virtual const TerrainCreateInfo& GetInfo() const noexcept = 0;
    [[nodiscard]] virtual bool IsCreated() const noexcept = 0;

    [[nodiscard]] virtual ITerrainComponent* GetComponent() noexcept = 0;
    [[nodiscard]] virtual ITerrainLOD& Lod() noexcept = 0;
    [[nodiscard]] virtual ITerrainMaterial& Materials() noexcept = 0;
    [[nodiscard]] virtual ITerrainCollision& Collision() noexcept = 0;
    [[nodiscard]] virtual ITerrainStreaming& Streaming() noexcept = 0;
    [[nodiscard]] virtual ITerrainRenderer& Renderer() noexcept = 0;
    [[nodiscard]] virtual ITerrainBrush& Brush() noexcept = 0;

    [[nodiscard]] virtual TerrainHeightmap& Heightfield() noexcept = 0;
    [[nodiscard]] virtual const TerrainHeightmap& Heightfield() const noexcept = 0;

    [[nodiscard]] virtual int GetChunkCount() const noexcept = 0;
    [[nodiscard]] virtual ITerrainChunk* GetChunk(int index) noexcept = 0;

    [[nodiscard]] virtual bool ApplyGenerator(const TerrainGeneratorParams& params) = 0;
    [[nodiscard]] virtual std::shared_ptr<ITerrainAsset> CaptureAsset(
        std::string_view displayName = "Landscape") const = 0;
    [[nodiscard]] virtual bool ApplyAsset(const ITerrainAsset& asset) = 0;

    [[nodiscard]] virtual std::vector<std::uint16_t> CaptureElevationSamples() const = 0;
    [[nodiscard]] virtual bool RestoreElevationSamples(
        const std::vector<std::uint16_t>& samples) = 0;

    virtual void Tick(
        float deltaSeconds,
        const we::math::Vec3& cameraWorldPos,
        const we::math::Mat4* viewProj) = 0;
};

} // namespace we::runtime::terrain
