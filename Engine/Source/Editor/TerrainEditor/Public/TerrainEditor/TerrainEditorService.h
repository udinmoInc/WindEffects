#pragma once

#include "TerrainEditor/Export.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::terrain {
enum class TerrainBrushOp;
enum class HeightmapFormat;
}

namespace we::editor::terrain {

// Editor-facing facade over runtime TerrainSystem (sculpt tools, import/export, foliage).
class TERRAINEDITOR_API TerrainEditorService {
public:
    static TerrainEditorService& Get();

    void BindScene(we::runtime::scene::Scene* scene);

    bool EnsureLandscape();
    bool GenerateDefaultLandscape();
    bool ImportHeightmap(const std::filesystem::path& path);
    bool ExportHeightmap(const std::filesystem::path& path);

    void SetBrushOp(we::runtime::terrain::TerrainBrushOp op);
    void SetBrushRadius(float radius);
    void SetBrushStrength(float strength);

    bool ApplyBrushAtWorld(float worldX, float worldZ);
    int SpawnFoliageInDefaultRegion();

    void Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ);

private:
    TerrainEditorService() = default;
    we::runtime::scene::Scene* m_Scene = nullptr;
};

} // namespace we::editor::terrain
