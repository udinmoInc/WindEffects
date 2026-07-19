#pragma once

#include "TerrainEditor/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainGenerators.h"
#include "Terrain/TerrainBrushSystem.h"
#include "Terrain/TerrainAsset.h"
#include "Terrain/ITerrainRuntime.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::scene {
class Scene;
}

namespace we::editor::undo {
class IUndoRuntime;
}

namespace we::editor::viewportedit {
class IViewportEditor;
}

namespace we::editor::terrain {

/// Parameters shown by the New Landscape dialog / creation wizard.
/// Heightmap import is optional — default creation is Flat (no heightmap file).
struct TERRAINEDITOR_API NewLandscapeDialogState {
    std::string name = "Landscape";
    runtime_terrain::TerrainCreateInfo createInfo{};
    runtime_terrain::TerrainCreationMethod creationMethod =
        runtime_terrain::TerrainCreationMethod::Flat;
    runtime_terrain::TerrainGeneratorId generatorId = runtime_terrain::TerrainGeneratorId::Flat;
    runtime_terrain::TerrainGeneratorParams generator{};
    std::filesystem::path importHeightmapPath; // optional
    std::string materialSlot0;
    bool enableStreaming = true;
    bool enableCollision = true;
    bool enableLod = true;
    bool enableEditLayers = true;

    /// Deprecated aliases kept for older UI bindings.
    bool generateProcedural = false;
    runtime_terrain::ProceduralHeightmapParams procedural{};
};

enum class LandscapeWizardStep : std::uint8_t {
    Size = 0,
    Generator,
    Materials,
    Streaming,
    Confirm,
};

/// Multi-step landscape creation wizard (data only — UI binds to this state).
class TERRAINEDITOR_API ILandscapeCreationWizard {
public:
    virtual ~ILandscapeCreationWizard() = default;

    [[nodiscard]] virtual LandscapeWizardStep Step() const noexcept = 0;
    [[nodiscard]] virtual NewLandscapeDialogState& State() noexcept = 0;
    [[nodiscard]] virtual const NewLandscapeDialogState& State() const noexcept = 0;

    virtual bool Next() = 0;
    virtual bool Back() = 0;
    [[nodiscard]] virtual bool CanFinish() const noexcept = 0;
    [[nodiscard]] virtual bool Finish() = 0;
    virtual void Reset() = 0;
};

struct TERRAINEDITOR_API LandscapeBrushPreview {
    float worldX = 0.f;
    float worldY = 0.f;
    float worldZ = 0.f;
    float radiusWorld = 16.f;
    float strength = 0.35f;
    float falloff = 0.5f;
    bool visible = false;
    runtime_terrain::TerrainBrushOp op = runtime_terrain::TerrainBrushOp::Raise;
};

enum class LandscapeBrushShape : std::uint8_t {
    Circle = 0,
    Square,
};

/// Editor-local brush UI flags (not Terrain Runtime state).
struct TERRAINEDITOR_API LandscapeBrushUiState {
    LandscapeBrushShape shape = LandscapeBrushShape::Circle;
    bool invert = false;
    bool mirror = false;
    bool showPreview = true;
    bool showCursor = true;
    std::string alphaPath; // placeholder path for custom alpha brush
};

struct TERRAINEDITOR_API LandscapeInfoSnapshot {
    bool exists = false;
    std::string name = "Landscape";
    int resolutionX = 0;
    int resolutionY = 0;
    int chunkCountX = 0;
    int chunkCountZ = 0;
    int chunkQuads = 0;
    int maxLod = 0;
    bool lodEnabled = false;
    bool streamingEnabled = false;
    bool collisionEnabled = false;
    std::string materialSlot0;
    std::uint64_t sampleMemoryBytes = 0;
    float worldSizeX = 0.f;
    float worldSizeY = 0.f;
    float heightScale = 0.f;
};

enum class LandscapeWorkspaceTab : std::uint8_t {
    Create = 0,
    Sculpt,
    Paint,
    Manage,
};

/// Editor-facing Landscape service — terrain is a world actor/asset, not viewport state.
class TERRAINEDITOR_API ILandscapeEditor {
public:
    virtual ~ILandscapeEditor() = default;

    virtual void BindScene(we::runtime::scene::Scene* scene) = 0;
    virtual void BindUndo(undo::IUndoRuntime* undo) = 0;
    virtual void BindViewport(viewportedit::IViewportEditor* viewport) = 0;
    virtual void BindTerrainRuntime(runtime_terrain::ITerrainRuntime* runtime) = 0;

    [[nodiscard]] virtual ILandscapeCreationWizard& Wizard() noexcept = 0;
    [[nodiscard]] virtual NewLandscapeDialogState& Dialog() noexcept = 0;

    /// Create Landscape from dialog — Flat by default, never requires a heightmap file.
    [[nodiscard]] virtual bool CreateFromDialog() = 0;
    [[nodiscard]] virtual bool EnsureLandscape() = 0;
    [[nodiscard]] virtual bool GenerateDefaultLandscape() = 0;
    [[nodiscard]] virtual bool GenerateProcedural(
        const runtime_terrain::ProceduralHeightmapParams& params) = 0;
    [[nodiscard]] virtual bool ApplyGenerator(
        const runtime_terrain::TerrainGeneratorParams& params) = 0;

    [[nodiscard]] virtual bool HasLandscape() const noexcept = 0;
    [[nodiscard]] virtual LandscapeInfoSnapshot LandscapeInfo() const = 0;
    [[nodiscard]] virtual bool DeleteLandscape() = 0;
    [[nodiscard]] virtual bool ResizeLandscape(int resolutionX, int resolutionY) = 0;
    [[nodiscard]] virtual bool RebuildMeshes() = 0;
    [[nodiscard]] virtual bool RebuildCollision() = 0;
    [[nodiscard]] virtual bool RebuildLOD() = 0;

    /// Optional heightmap file import/export.
    [[nodiscard]] virtual bool ImportHeightmap(const std::filesystem::path& path) = 0;
    [[nodiscard]] virtual bool ExportHeightmap(
        const std::filesystem::path& path,
        runtime_terrain::HeightmapFormat format =
            runtime_terrain::HeightmapFormat::Raw16Le) = 0;

    [[nodiscard]] virtual runtime_terrain::TerrainAsset CaptureAsset() const = 0;
    [[nodiscard]] virtual bool ApplyAsset(const runtime_terrain::TerrainAsset& asset) = 0;

    virtual void SetBrushOp(runtime_terrain::TerrainBrushOp op) = 0;
    virtual void SetBrushRadius(float radius) = 0;
    virtual void SetBrushStrength(float strength) = 0;
    virtual void SetBrushFalloff(float falloff) = 0;
    [[nodiscard]] virtual const runtime_terrain::TerrainBrushSettings& BrushSettings() const = 0;

    [[nodiscard]] virtual LandscapeBrushUiState& BrushUi() noexcept = 0;
    [[nodiscard]] virtual const LandscapeBrushUiState& BrushUi() const noexcept = 0;
    virtual void ClearBrushAlpha() = 0;
    virtual void SetBrushAlphaPlaceholder(std::string_view path) = 0;

    [[nodiscard]] virtual bool ApplyBrushAtWorld(float worldX, float worldZ) = 0;
    [[nodiscard]] virtual bool BeginBrushStroke() = 0;
    [[nodiscard]] virtual bool EndBrushStroke() = 0;

    [[nodiscard]] virtual int GetLayerCount() const noexcept = 0;
    [[nodiscard]] virtual std::string GetLayerName(int index) const = 0;
    virtual void SetLayerName(int index, std::string_view name) = 0;
    [[nodiscard]] virtual std::string GetLayerMaterialPath(int index) const = 0;
    virtual void SetLayerMaterialPath(int index, std::string_view path) = 0;
    [[nodiscard]] virtual bool AddLayer(std::string_view name = "Layer") = 0;
    [[nodiscard]] virtual bool RemoveLayer(int index) = 0;
    [[nodiscard]] virtual int ActivePaintLayer() const noexcept = 0;

    virtual void SetPaintLayer(int layerIndex) = 0;
    [[nodiscard]] virtual bool PaintLayerAtWorld(float worldX, float worldZ) = 0;

    virtual void SelectTerrainActor() = 0;
    [[nodiscard]] virtual std::uint64_t LandscapeEntityId() const noexcept = 0;

    [[nodiscard]] virtual LandscapeBrushPreview& BrushPreview() noexcept = 0;
    virtual void UpdateBrushPreview(float worldX, float worldY, float worldZ) = 0;

    virtual void Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ) = 0;

    virtual void InstallViewportMode() = 0;
};

[[nodiscard]] TERRAINEDITOR_API ILandscapeEditor& GetLandscapeEditor() noexcept;

/// Backward-compatible facade.
class TERRAINEDITOR_API TerrainEditorService {
public:
    static TerrainEditorService& Get();

    void BindScene(we::runtime::scene::Scene* scene);
    void BindUndo(undo::IUndoRuntime* undo);
    void BindViewport(viewportedit::IViewportEditor* viewport);

    bool EnsureLandscape();
    bool GenerateDefaultLandscape();
    bool ImportHeightmap(const std::filesystem::path& path);
    bool ExportHeightmap(const std::filesystem::path& path);

    void SetBrushOp(runtime_terrain::TerrainBrushOp op);
    void SetBrushRadius(float radius);
    void SetBrushStrength(float strength);

    bool ApplyBrushAtWorld(float worldX, float worldZ);
    int SpawnFoliageInDefaultRegion();

    void Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ);
    void InstallViewportMode();

    [[nodiscard]] ILandscapeEditor& Landscape() noexcept;
};

} // namespace we::editor::terrain
