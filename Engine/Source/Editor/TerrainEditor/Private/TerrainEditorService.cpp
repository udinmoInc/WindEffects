#include "TerrainEditor/TerrainEditorService.h"

#include "Terrain/TerrainSystem.h"
#include "Terrain/TerrainTypes.h"
#include "Core/Logger.h"
#include "Scene/Scene.h"

#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::editor::terrain {

TerrainEditorService& TerrainEditorService::Get() {
    static TerrainEditorService instance;
    return instance;
}

void TerrainEditorService::BindScene(we::runtime::scene::Scene* scene) {
    m_Scene = scene;
    we::runtime::terrain::TerrainSystem::Get().BindScene(scene);
}

bool TerrainEditorService::EnsureLandscape() {
    auto& terrain = we::runtime::terrain::TerrainSystem::Get();
    if (m_Scene) {
        terrain.BindScene(m_Scene);
    }
    if (!terrain.IsCreated()) {
        return GenerateDefaultLandscape();
    }
    if (terrain.LandscapeEntityId() == 0) {
        terrain.SpawnLandscapeActor("Landscape");
    }
    return true;
}

bool TerrainEditorService::GenerateDefaultLandscape() {
    auto& terrain = we::runtime::terrain::TerrainSystem::Get();
    if (m_Scene) {
        terrain.BindScene(m_Scene);
    }
    we::runtime::terrain::TerrainCreateInfo info{};
    if (!terrain.Create(info)) {
        return false;
    }
    terrain.SpawnLandscapeActor("Landscape");
    HE_INFO("[TerrainEditor] Generated default Landscape.");
    return true;
}

bool TerrainEditorService::ImportHeightmap(const std::filesystem::path& path) {
    EnsureLandscape();
    auto& terrain = we::runtime::terrain::TerrainSystem::Get();
    if (!terrain.ImportHeightmap(path)) {
        HE_ERROR("[TerrainEditor] Heightmap import failed: " + path.string());
        return false;
    }
    terrain.SpawnLandscapeActor("Landscape");
    HE_INFO("[TerrainEditor] Imported heightmap: " + path.string());
    return true;
}

bool TerrainEditorService::ExportHeightmap(const std::filesystem::path& path) {
    if (!EnsureLandscape()) {
        return false;
    }
    const auto format = we::runtime::terrain::HeightmapFormat::Raw16Le;
    return we::runtime::terrain::TerrainSystem::Get().ExportHeightmap(path, format);
}

void TerrainEditorService::SetBrushOp(we::runtime::terrain::TerrainBrushOp op) {
    auto settings = we::runtime::terrain::TerrainSystem::Get().Brushes().Settings();
    settings.op = op;
    we::runtime::terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
}

void TerrainEditorService::SetBrushRadius(float radius) {
    auto settings = we::runtime::terrain::TerrainSystem::Get().Brushes().Settings();
    settings.radius = radius;
    we::runtime::terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
}

void TerrainEditorService::SetBrushStrength(float strength) {
    auto settings = we::runtime::terrain::TerrainSystem::Get().Brushes().Settings();
    settings.strength = strength;
    we::runtime::terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
}

bool TerrainEditorService::ApplyBrushAtWorld(float worldX, float worldZ) {
    if (!EnsureLandscape()) {
        return false;
    }
    return we::runtime::terrain::TerrainSystem::Get().ApplyBrushWorld(worldX, worldZ);
}

int TerrainEditorService::SpawnFoliageInDefaultRegion() {
    if (!EnsureLandscape()) {
        return 0;
    }
    auto& terrain = we::runtime::terrain::TerrainSystem::Get();
    we::runtime::terrain::FoliageSpawnParams params{};
    params.densityPerSquareMeter = 0.02f;
    params.maxSlopeDegrees = 30.0f;
    const float half = terrain.Info().worldSizeX * 0.35f;
    return terrain.Foliage().Spawn(terrain.Collision(), terrain.Info(), &terrain.Materials(),
        params, glm::vec2(-half, -half), glm::vec2(half, half));
}

void TerrainEditorService::Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ) {
    auto& terrain = we::runtime::terrain::TerrainSystem::Get();
    if (!terrain.IsCreated()) {
        return;
    }
    terrain.Tick(deltaSeconds, glm::vec3(cameraX, cameraY, cameraZ), nullptr);
}

} // namespace we::editor::terrain
