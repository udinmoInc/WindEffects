#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainHeightmap.h"
#include "Terrain/TerrainChunkManager.h"
#include "Terrain/TerrainLODManager.h"
#include "Terrain/TerrainCollision.h"
#include "Terrain/TerrainMaterialSystem.h"
#include "Terrain/TerrainBrushSystem.h"
#include "Terrain/TerrainStreaming.h"
#include "Terrain/TerrainRenderer.h"
#include "Terrain/TerrainFoliage.h"
#include "Terrain/TerrainGenerators.h"
#include "Terrain/TerrainAsset.h"
#include "Terrain/ITerrainRuntime.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace we::runtime::scene {
class Scene;
}

namespace we::rhi {
class IRHIDevice;
}

namespace we::runtime::terrain {

/// Legacy facade over ITerrainRuntime active landscape.
/// Prefer CreateTerrainRuntime / ITerrainManager for new code.
/// Create() always produces an editable landscape without requiring a heightmap file.
class TERRAIN_API TerrainSystem {
public:
    static TerrainSystem& Get();

    bool Create(const TerrainCreateInfo& info);
    void Destroy();
    bool IsCreated() const;

    void BindScene(we::runtime::scene::Scene* scene);
    void BindRenderer(we::rhi::IRHIDevice* device);

    std::uint64_t SpawnLandscapeActor(const char* name = "Landscape");

    const TerrainCreateInfo& Info() const;
    TerrainHeightmap& Heightmap();
    const TerrainHeightmap& Heightmap() const;

    TerrainChunkManager& Chunks();
    TerrainLODManager& Lod();
    TerrainCollision& Collision();
    TerrainMaterialSystem& Materials();
    TerrainBrushSystem& Brushes();
    TerrainStreaming& Streaming();
    TerrainRenderer& Renderer();
    TerrainFoliageSystem& Foliage();

    bool ImportHeightmap(const std::filesystem::path& path);
    bool ExportHeightmap(const std::filesystem::path& path, HeightmapFormat format);

    bool GenerateProcedural(const ProceduralHeightmapParams& params);
    bool ApplyGenerator(const TerrainGeneratorParams& params);
    [[nodiscard]] TerrainAsset CaptureAsset(std::string_view displayName = "Landscape") const;
    bool ApplyAsset(const TerrainAsset& asset);

    [[nodiscard]] std::vector<std::uint16_t> CaptureHeightSamples() const;
    bool RestoreHeightSamples(const std::vector<std::uint16_t>& samples);

    bool ApplyBrushWorld(float worldX, float worldZ);

    void Tick(float deltaSeconds, const we::math::Vec3& cameraWorldPos, const we::math::Mat4* viewProj);

    std::uint64_t LandscapeEntityId() const;
    [[nodiscard]] TerrainId ActiveTerrainId() const noexcept;

private:
    TerrainSystem() = default;
};

} // namespace we::runtime::terrain

