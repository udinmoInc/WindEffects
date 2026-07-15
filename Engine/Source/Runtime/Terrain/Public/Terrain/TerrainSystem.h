#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

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

#include <cstdint>
#include <filesystem>
#include <memory>

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::renderer {
class SceneRenderer;
class DeviceContext;
class ResourceManager;
}

namespace we::runtime::terrain {

// Facade owning Landscape-style subsystems. Extensible for open-world streaming,
// virtual texturing, water, GPU clipmaps without restructuring callers.
class TERRAIN_API TerrainSystem {
public:
    static TerrainSystem& Get();

    bool Create(const TerrainCreateInfo& info);
    void Destroy();
    bool IsCreated() const { return m_Created; }

    void BindScene(we::runtime::scene::Scene* scene);
    void BindRenderer(we::runtime::renderer::SceneRenderer* renderer,
        we::runtime::renderer::DeviceContext* device,
        we::runtime::renderer::ResourceManager* resources);

    std::uint64_t SpawnLandscapeActor(const char* name = "Landscape");

    const TerrainCreateInfo& Info() const { return m_Info; }
    TerrainHeightmap& Heightmap() { return m_Heightmap; }
    const TerrainHeightmap& Heightmap() const { return m_Heightmap; }

    TerrainChunkManager& Chunks() { return m_Chunks; }
    TerrainLODManager& Lod() { return m_Lod; }
    TerrainCollision& Collision() { return m_Collision; }
    TerrainMaterialSystem& Materials() { return m_Materials; }
    TerrainBrushSystem& Brushes() { return m_Brushes; }
    TerrainStreaming& Streaming() { return m_Streaming; }
    TerrainRenderer& Renderer() { return m_Renderer; }
    TerrainFoliageSystem& Foliage() { return m_Foliage; }

    bool ImportHeightmap(const std::filesystem::path& path);
    bool ExportHeightmap(const std::filesystem::path& path, HeightmapFormat format);

    // Sculpt at world XZ; marks dirty chunks and rebuilds meshes/collision for those only.
    bool ApplyBrushWorld(float worldX, float worldZ);

    void Tick(float deltaSeconds, const glm::vec3& cameraWorldPos, const glm::mat4* viewProj);

    std::uint64_t LandscapeEntityId() const { return m_LandscapeEntityId; }

private:
    TerrainSystem() = default;

    void RebuildDirty();

    TerrainCreateInfo m_Info{};
    TerrainHeightmap m_Heightmap;
    TerrainChunkManager m_Chunks;
    TerrainLODManager m_Lod;
    TerrainCollision m_Collision;
    TerrainMaterialSystem m_Materials;
    TerrainBrushSystem m_Brushes;
    TerrainStreaming m_Streaming;
    TerrainRenderer m_Renderer;
    TerrainFoliageSystem m_Foliage;

    we::runtime::scene::Scene* m_Scene = nullptr;
    we::runtime::renderer::SceneRenderer* m_SceneRenderer = nullptr;
    std::uint64_t m_LandscapeEntityId = 0;
    bool m_Created = false;
};

} // namespace we::runtime::terrain

#pragma warning(pop)
