#include "TerrainEditor/ILandscapeEditor.h"
#include "TerrainEditor/TerrainEditorDiagnostics.h"

#include "Terrain/TerrainSystem.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainReflection.h"
#include "ViewportEdit/ViewportEdit.h"
#include "Undo/IUndoRuntime.h"
#include "Undo/ITransactionManager.h"
#include "Undo/UndoTypes.h"
#include "Reflection/ITypeRegistry.h"
#include "Core/Logger.h"
#include "Core/Math/Types.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "EditorCamera.h"
#include <sstream>
#include <span>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace we::editor::terrain {
namespace {

using viewportedit::IViewportContext;
using viewportedit::IViewportEditor;
using viewportedit::IViewportMode;
using viewportedit::IViewportOverlay;
using viewportedit::IViewportRenderExtension;
using viewportedit::IViewportTool;
using viewportedit::ViewportEditDiagnostics;
using viewportedit::ViewportInputEvent;
using viewportedit::ViewportModeDescriptor;
using viewportedit::ViewportModeId;
using viewportedit::ViewportMouseButton;
using viewportedit::ViewportObjectId;
using viewportedit::ViewportOverlayPass;
using viewportedit::ViewportToolId;
using viewportedit::GetViewportModeRegistry;

class LandscapeCreationWizard final : public ILandscapeCreationWizard {
public:
    [[nodiscard]] LandscapeWizardStep Step() const noexcept override { return m_Step; }
    [[nodiscard]] NewLandscapeDialogState& State() noexcept override { return m_State; }
    [[nodiscard]] const NewLandscapeDialogState& State() const noexcept override { return m_State; }

    bool Next() override {
        if (m_Step == LandscapeWizardStep::Confirm) {
            return false;
        }
        m_Step = static_cast<LandscapeWizardStep>(static_cast<std::uint8_t>(m_Step) + 1);
        return true;
    }

    bool Back() override {
        if (m_Step == LandscapeWizardStep::Size) {
            return false;
        }
        m_Step = static_cast<LandscapeWizardStep>(static_cast<std::uint8_t>(m_Step) - 1);
        return true;
    }

    [[nodiscard]] bool CanFinish() const noexcept override {
        // Heightmap path is never required — Flat/Empty/Procedural are always valid.
        return m_State.createInfo.resolutionX > 1 && m_State.createInfo.resolutionY > 1
            && !m_State.name.empty();
    }

    [[nodiscard]] bool Finish() override {
        return CanFinish();
    }

    void Reset() override {
        m_Step = LandscapeWizardStep::Size;
        m_State = NewLandscapeDialogState{};
        m_State.creationMethod = runtime_terrain::TerrainCreationMethod::Flat;
        m_State.generatorId = runtime_terrain::TerrainGeneratorId::Flat;
    }

private:
    LandscapeWizardStep m_Step = LandscapeWizardStep::Size;
    NewLandscapeDialogState m_State{};
};

class LandscapeBrushOverlay final : public IViewportOverlay {
public:
    explicit LandscapeBrushOverlay(LandscapeBrushPreview& preview)
        : m_Preview(preview)
    {}

    [[nodiscard]] std::string_view GetId() const noexcept override { return "Landscape.BrushPreview"; }
    [[nodiscard]] ViewportOverlayPass Pass() const noexcept override {
        return ViewportOverlayPass::BrushPreview;
    }
    [[nodiscard]] bool IsEnabled() const noexcept override { return m_Preview.visible; }

    void Draw(IViewportContext&, float, float) override {
        // Host/renderer draws circle from BrushPreview (GPU brush preview hook).
    }

private:
    LandscapeBrushPreview& m_Preview;
};

class LandscapeRenderExtension final : public IViewportRenderExtension {
public:
    explicit LandscapeRenderExtension(LandscapeBrushPreview& preview)
        : m_Preview(preview)
    {}

    [[nodiscard]] std::string_view GetId() const noexcept override {
        return "Landscape.GPUBrushPreview";
    }
    [[nodiscard]] int SortOrder() const noexcept override { return 50; }

    void OnDrawOverlay(IViewportContext&) override {
        // Future: dispatch GPU brush cursor / falloff ring via Renderer hooks.
        (void)m_Preview;
    }

private:
    LandscapeBrushPreview& m_Preview;
};

class LandscapeSculptTool final : public IViewportTool {
public:
    LandscapeSculptTool(ILandscapeEditor& editor, ViewportToolId id, std::string_view name,
        runtime_terrain::TerrainBrushOp op)
        : m_Editor(editor)
        , m_Id(id)
        , m_Name(name)
        , m_Op(op)
    {}

    [[nodiscard]] ViewportToolId GetId() const noexcept override { return m_Id; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return m_Name; }

    void OnActivated(IViewportContext&) override {
        m_Editor.SetBrushOp(m_Op);
        ViewportEditDiagnostics::Get().OnToolActivated();
    }
    void OnDeactivated(IViewportContext&) override {}
    void Tick(IViewportContext&, float) override {}

    [[nodiscard]] bool OnMouseDown(IViewportContext& context, const ViewportInputEvent& e) override {
        if (e.button != ViewportMouseButton::Left || e.alt) {
            return false;
        }
        (void)m_Editor.BeginBrushStroke();
        m_Painting = true;
        ApplyAtCursor(context, e.x, e.y);
        return true;
    }

    [[nodiscard]] bool OnMouseUp(IViewportContext&, const ViewportInputEvent& e) override {
        if (!m_Painting || e.button != ViewportMouseButton::Left) {
            return false;
        }
        m_Painting = false;
        (void)m_Editor.EndBrushStroke();
        return true;
    }

    [[nodiscard]] bool OnMouseMove(IViewportContext& context, const ViewportInputEvent& e) override {
        ApplyPreview(context, e.x, e.y);
        if (!m_Painting) {
            return false;
        }
        ApplyAtCursor(context, e.x, e.y);
        return true;
    }

    [[nodiscard]] bool OnKeyDown(IViewportContext&, const ViewportInputEvent&) override {
        return false;
    }
    [[nodiscard]] bool OnKeyUp(IViewportContext&, const ViewportInputEvent&) override {
        return false;
    }

private:
    void ApplyPreview(IViewportContext& context, float x, float y) {
        const auto hit = context.HitTester().Pick(
            x, y, context.ViewportWidth(), context.ViewportHeight());
        if (hit.valid) {
            m_Editor.UpdateBrushPreview(hit.worldPoint.x, hit.worldPoint.y, hit.worldPoint.z);
        } else {
            // Project onto ground plane Y=0 when no actor hit.
            const auto ray = context.HitTester().ScreenToRay(
                x, y, context.ViewportWidth(), context.ViewportHeight());
            if (std::abs(ray.direction.y) > 1e-5f) {
                const float t = -ray.origin.y / ray.direction.y;
                if (t > 0.f) {
                    m_Editor.UpdateBrushPreview(
                        ray.origin.x + ray.direction.x * t,
                        0.f,
                        ray.origin.z + ray.direction.z * t);
                }
            }
        }
    }

    void ApplyAtCursor(IViewportContext& context, float x, float y) {
        ApplyPreview(context, x, y);
        const auto& preview = m_Editor.BrushPreview();
        if (preview.visible) {
            (void)m_Editor.ApplyBrushAtWorld(preview.worldX, preview.worldZ);
        }
    }

    ILandscapeEditor& m_Editor;
    ViewportToolId m_Id;
    std::string_view m_Name;
    runtime_terrain::TerrainBrushOp m_Op;
    bool m_Painting = false;
};

class LandscapeMode final : public IViewportMode {
public:
    explicit LandscapeMode(ILandscapeEditor& editor)
        : m_Editor(editor)
    {
        m_Tools = {
            ViewportToolId::LandscapeSculpt,
            ViewportToolId::LandscapeSmooth,
            ViewportToolId::LandscapeFlatten,
            ViewportToolId::LandscapeNoise,
            ViewportToolId::LandscapePaint,
            ViewportToolId::Select,
        };
    }

    [[nodiscard]] ViewportModeId GetId() const noexcept override {
        return ViewportModeId::Landscape;
    }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return "Landscape"; }

    void OnEnter(IViewportContext& context) override {
        TerrainEditorDiagnostics::Get().OnModeEnter();
        (void)m_Editor.EnsureLandscape();
        if (m_Editor.LandscapeEntityId() != 0) {
            if (auto* selCtx = context.SelectionContextPtr()) {
                selCtx->SetPrimaryTerrain(ViewportObjectId{m_Editor.LandscapeEntityId()});
            }
            m_Editor.SelectTerrainActor();
        }
        (void)context;
    }

    void OnExit(IViewportContext& context) override {
        m_Editor.BrushPreview().visible = false;
        if (auto* selCtx = context.SelectionContextPtr()) {
            selCtx->ClearFilter();
        }
    }

    void Tick(IViewportContext&, float) override {}

    [[nodiscard]] std::span<const ViewportToolId> DefaultTools() const noexcept override {
        return m_Tools;
    }

    [[nodiscard]] ViewportToolId PreferredTool() const noexcept override {
        return ViewportToolId::LandscapeSculpt;
    }

private:
    ILandscapeEditor& m_Editor;
    std::vector<ViewportToolId> m_Tools;
};

class LandscapeEditorImpl final : public ILandscapeEditor {
public:
    static LandscapeEditorImpl& Get() {
        static LandscapeEditorImpl instance;
        return instance;
    }

    void BindScene(scene::Scene* scene) override {
        m_Scene = scene;
        runtime_terrain::TerrainSystem::Get().BindScene(scene);
    }

    void BindUndo(undo::IUndoRuntime* undo) override {
        m_Undo = undo;
        if (m_TerrainRuntime && m_Undo) {
            WireSessionUndo();
        }
    }

    void BindViewport(IViewportEditor* viewport) override {
        m_Viewport = viewport;
        if (m_Viewport && !m_Installed) {
            InstallViewportMode();
        }
    }

    void BindTerrainRuntime(runtime_terrain::ITerrainRuntime* runtime) override {
        m_TerrainRuntime = runtime;
        if (m_TerrainRuntime && m_Undo) {
            WireSessionUndo();
        }
    }

    [[nodiscard]] ILandscapeCreationWizard& Wizard() noexcept override { return m_Wizard; }
    [[nodiscard]] NewLandscapeDialogState& Dialog() noexcept override { return m_Dialog; }

    [[nodiscard]] bool CreateFromDialog() override {
        if (!m_Wizard.CanFinish() && !m_Dialog.name.empty()) {
            m_Wizard.State() = m_Dialog;
        }
        if (!m_Wizard.Finish()) {
            return false;
        }
        const auto& state = m_Wizard.State();
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (m_Scene) {
            terrain.BindScene(m_Scene);
        }

        runtime_terrain::TerrainCreateInfo info = state.createInfo;
        info.displayName = state.name;
        info.creationMethod = state.creationMethod;
        info.generator = state.generator;
        info.generator.generator = state.generatorId;
        info.streaming.enabled = state.enableStreaming;
        info.collision.enabled = state.enableCollision;
        info.lod.enabled = state.enableLod;
        if (!state.materialSlot0.empty()) {
            info.materialSlot0 = state.materialSlot0;
        }

        // Backward-compat: old generateProcedural flag.
        if (state.generateProcedural) {
            info.creationMethod = runtime_terrain::TerrainCreationMethod::Procedural;
            info.generator = state.procedural;
            if (info.generator.generator == runtime_terrain::TerrainGeneratorId::Flat) {
                info.generator.generator = runtime_terrain::TerrainGeneratorId::Fbm;
            }
        }

        // Heightmap import is optional and never blocks Create Landscape.
        if (info.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport
            && state.importHeightmapPath.empty())
        {
            info.creationMethod = runtime_terrain::TerrainCreationMethod::Flat;
            info.generator.generator = runtime_terrain::TerrainGeneratorId::Flat;
        }

        if (!terrain.Create(info)) {
            return false;
        }

        if (info.creationMethod == runtime_terrain::TerrainCreationMethod::HeightmapImport
            && !state.importHeightmapPath.empty())
        {
            (void)terrain.ImportHeightmap(state.importHeightmapPath);
        }

        terrain.Streaming().SetEnabled(state.enableStreaming);
        terrain.Streaming().SetLoadRadiusChunks(state.enableStreaming ? 8 : 64);
        // Keep initial landscape fully resident so Create always yields visible geometry.
        for (auto& chunk : terrain.Chunks().Chunks()) {
            chunk.visible = true;
            terrain.Streaming().RequestLoad(chunk.id);
        }
        const std::uint64_t entityId = terrain.SpawnLandscapeActor(state.name.c_str());
        if (entityId == 0) {
            HE_ERROR("[TerrainEditor] Create Landscape failed at stage=SpawnLandscapeActor"
                " reason=entity_id_zero");
            terrain.Destroy();
            return false;
        }

        runtime_terrain::TerrainCreationReport report = VerifyRenderableLandscape("CreateFromDialog");
        report.entityId = entityId;
        report.terrainId = terrain.ActiveTerrainId();
        LogCreationReport(report);
        if (!report.success) {
            HE_ERROR("[TerrainEditor] Create Landscape aborted — not renderable: stage="
                + report.failedStage + " reason=" + report.reason);
            terrain.Destroy();
            return false;
        }

        SelectTerrainActor();
        FrameLandscapeInViewport();
        TerrainEditorDiagnostics::Get().OnLandscapeCreated();
        m_Dialog = state;
        HE_INFO("[TerrainEditor] Created Landscape '" + state.name
            + "' with visible terrain geometry.");
        return true;
    }

    [[nodiscard]] bool EnsureLandscape() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
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

    [[nodiscard]] bool GenerateDefaultLandscape() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (m_Scene) {
            terrain.BindScene(m_Scene);
        }
        runtime_terrain::TerrainCreateInfo info{};
        info.creationMethod = runtime_terrain::TerrainCreationMethod::Flat;
        info.generator.generator = runtime_terrain::TerrainGeneratorId::Flat;
        info.initialElevation = 0.5f;
        info.displayName = "Landscape";
        if (!terrain.Create(info)) {
            return false;
        }
        for (auto& chunk : terrain.Chunks().Chunks()) {
            chunk.visible = true;
        }
        if (terrain.SpawnLandscapeActor("Landscape") == 0) {
            HE_ERROR("[TerrainEditor] GenerateDefaultLandscape failed: spawn");
            terrain.Destroy();
            return false;
        }
        const auto report = VerifyRenderableLandscape("GenerateDefaultLandscape");
        LogCreationReport(report);
        if (!report.success) {
            HE_ERROR("[TerrainEditor] GenerateDefaultLandscape not renderable: "
                + report.failedStage + " / " + report.reason);
            terrain.Destroy();
            return false;
        }
        SelectTerrainActor();
        FrameLandscapeInViewport();
        TerrainEditorDiagnostics::Get().OnLandscapeCreated();
        HE_INFO("[TerrainEditor] Generated default Flat Landscape (no heightmap required).");
        return true;
    }

    [[nodiscard]] bool GenerateProcedural(
        const runtime_terrain::ProceduralHeightmapParams& params) override
    {
        return ApplyGenerator(params);
    }

    [[nodiscard]] bool ApplyGenerator(
        const runtime_terrain::TerrainGeneratorParams& params) override
    {
        if (!EnsureLandscape()) {
            return false;
        }
        return runtime_terrain::TerrainSystem::Get().ApplyGenerator(params);
    }

    [[nodiscard]] bool ImportHeightmap(const std::filesystem::path& path) override {
        (void)EnsureLandscape();
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.ImportHeightmap(path)) {
            HE_ERROR("[TerrainEditor] Heightmap import failed: " + path.string());
            return false;
        }
        terrain.SpawnLandscapeActor("Landscape");
        TerrainEditorDiagnostics::Get().OnHeightmapImported();
        HE_INFO("[TerrainEditor] Imported heightmap: " + path.string());
        return true;
    }

    [[nodiscard]] bool ExportHeightmap(
        const std::filesystem::path& path,
        runtime_terrain::HeightmapFormat format) override
    {
        if (!EnsureLandscape()) {
            return false;
        }
        const bool ok = runtime_terrain::TerrainSystem::Get().ExportHeightmap(path, format);
        if (ok) {
            TerrainEditorDiagnostics::Get().OnHeightmapExported();
        }
        return ok;
    }

    [[nodiscard]] runtime_terrain::TerrainAsset CaptureAsset() const override {
        return runtime_terrain::TerrainSystem::Get().CaptureAsset("Landscape");
    }

    [[nodiscard]] bool ApplyAsset(const runtime_terrain::TerrainAsset& asset) override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (m_Scene) {
            terrain.BindScene(m_Scene);
        }
        if (!terrain.ApplyAsset(asset)) {
            return false;
        }
        terrain.SpawnLandscapeActor(asset.Desc().displayName.c_str());
        return true;
    }

    void SetBrushOp(runtime_terrain::TerrainBrushOp op) override {
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.op = op;
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
        m_BrushPreview.op = op;
    }

    void SetBrushRadius(float radius) override {
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.radius = std::max(0.5f, radius);
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
        m_BrushPreview.radiusWorld = settings.radius;
    }

    void SetBrushStrength(float strength) override {
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.strength = std::clamp(strength, 0.f, 1.f);
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
        m_BrushPreview.strength = settings.strength;
    }

    void SetBrushFalloff(float falloff) override {
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.falloff = std::clamp(falloff, 0.f, 1.f);
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
        m_BrushPreview.falloff = settings.falloff;
    }

    [[nodiscard]] const runtime_terrain::TerrainBrushSettings& BrushSettings() const override {
        return runtime_terrain::TerrainSystem::Get().Brushes().Settings();
    }

    [[nodiscard]] bool BeginBrushStroke() override {
        m_StrokeBefore = runtime_terrain::TerrainSystem::Get().CaptureHeightSamples();
        m_StrokeActive = !m_StrokeBefore.empty();
        return m_StrokeActive;
    }

    [[nodiscard]] bool EndBrushStroke() override {
        if (!m_StrokeActive) {
            return false;
        }
        m_StrokeActive = false;
        auto after = runtime_terrain::TerrainSystem::Get().CaptureHeightSamples();
        if (after == m_StrokeBefore) {
            m_StrokeBefore.clear();
            return false;
        }
        auto before = std::move(m_StrokeBefore);
        m_StrokeBefore.clear();
        TerrainEditorDiagnostics::Get().OnBrushStroke();

        if (!m_Undo) {
            return true;
        }
        const bool ok = m_Undo->Manager().RecordCustom(
            "Landscape Brush",
            undo::TransactionKind::Generic,
            "landscape.brush",
            [before]() {
                return runtime_terrain::TerrainSystem::Get().RestoreHeightSamples(before);
            },
            [after = std::move(after)]() {
                return runtime_terrain::TerrainSystem::Get().RestoreHeightSamples(after);
            });
        if (ok) {
            TerrainEditorDiagnostics::Get().OnUndoTransaction();
        }
        return ok;
    }

    [[nodiscard]] bool HasLandscape() const noexcept override {
        return runtime_terrain::TerrainSystem::Get().IsCreated();
    }

    [[nodiscard]] LandscapeInfoSnapshot LandscapeInfo() const override {
        LandscapeInfoSnapshot snap{};
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        snap.exists = terrain.IsCreated();
        if (!snap.exists) {
            return snap;
        }
        const auto& info = terrain.Info();
        snap.name = info.displayName.empty() ? "Landscape" : info.displayName;
        snap.resolutionX = info.resolutionX;
        snap.resolutionY = info.resolutionY;
        snap.chunkCountX = terrain.Chunks().ChunkCountX();
        snap.chunkCountZ = terrain.Chunks().ChunkCountZ();
        snap.chunkQuads = terrain.Chunks().ChunkQuads();
        snap.maxLod = terrain.Lod().MaxLod();
        snap.lodEnabled = info.lod.enabled;
        snap.streamingEnabled = terrain.Streaming().IsEnabled();
        snap.collisionEnabled = info.collision.enabled;
        snap.materialSlot0 = info.materialSlot0;
        snap.worldSizeX = info.worldSizeX;
        snap.worldSizeY = info.worldSizeY;
        snap.heightScale = info.heightScale;
        const auto samples = static_cast<std::uint64_t>(
            std::max(0, info.resolutionX) * std::max(0, info.resolutionY));
        snap.sampleMemoryBytes = samples * sizeof(std::uint16_t);
        return snap;
    }

    [[nodiscard]] bool DeleteLandscape() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return false;
        }
        terrain.Destroy();
        HE_INFO("[TerrainEditor] Landscape deleted.");
        return true;
    }

    [[nodiscard]] bool ResizeLandscape(int resolutionX, int resolutionY) override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return false;
        }
        resolutionX = std::max(2, resolutionX);
        resolutionY = std::max(2, resolutionY);
        const std::uint16_t fill = static_cast<std::uint16_t>(
            std::clamp(terrain.Info().initialElevation, 0.f, 1.f) * 65535.f);
        terrain.Heightmap().Resize(resolutionX, resolutionY, fill);
        terrain.Materials().Initialize(resolutionX, resolutionY);
        terrain.Chunks().Initialize(terrain.Info());
        terrain.Chunks().MarkAllDirty();
        (void)RebuildMeshes();
        (void)RebuildCollision();
        return true;
    }

    [[nodiscard]] bool RebuildMeshes() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return false;
        }
        terrain.Chunks().MarkAllDirty();
        const int rebuilt = terrain.Chunks().RebuildDirtyMeshes(
            terrain.Heightmap(), terrain.Info(), terrain.Lod());
        return rebuilt >= 0;
    }

    [[nodiscard]] bool RebuildCollision() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return false;
        }
        terrain.Collision().RebuildDirty(terrain.Chunks());
        return true;
    }

    [[nodiscard]] bool RebuildLOD() override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return false;
        }
        terrain.Lod().UpdateChunkLods(
            terrain.Chunks(),
            we::math::Vec3(0.f, 100.f, 0.f),
            terrain.Info(),
            terrain.Heightmap().Width());
        return RebuildMeshes();
    }

    [[nodiscard]] LandscapeBrushUiState& BrushUi() noexcept override { return m_BrushUi; }
    [[nodiscard]] const LandscapeBrushUiState& BrushUi() const noexcept override { return m_BrushUi; }

    void ClearBrushAlpha() override {
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.alphaMask.clear();
        settings.alphaWidth = 0;
        settings.alphaHeight = 0;
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
        m_BrushUi.alphaPath.clear();
    }

    void SetBrushAlphaPlaceholder(std::string_view path) override {
        m_BrushUi.alphaPath = std::string(path);
        // Placeholder: mark CustomAlpha op readiness; actual mask load is host-provided later.
        if (!m_BrushUi.alphaPath.empty()) {
            auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
            if (settings.alphaMask.empty()) {
                settings.alphaWidth = 1;
                settings.alphaHeight = 1;
                settings.alphaMask.assign(1, 255);
                runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
            }
        }
    }

    [[nodiscard]] int GetLayerCount() const noexcept override {
        return m_LayerCount;
    }

    [[nodiscard]] std::string GetLayerName(int index) const override {
        if (index < 0 || index >= m_LayerCount) {
            return {};
        }
        if (!runtime_terrain::TerrainSystem::Get().IsCreated()) {
            return "Layer " + std::to_string(index);
        }
        return runtime_terrain::TerrainSystem::Get().Materials().Layer(index).name;
    }

    void SetLayerName(int index, std::string_view name) override {
        if (index < 0 || index >= m_LayerCount) {
            return;
        }
        if (!runtime_terrain::TerrainSystem::Get().IsCreated()) {
            return;
        }
        runtime_terrain::TerrainSystem::Get().Materials().Layer(index).name = std::string(name);
    }

    [[nodiscard]] std::string GetLayerMaterialPath(int index) const override {
        if (index < 0 || index >= m_LayerCount) {
            return {};
        }
        if (!runtime_terrain::TerrainSystem::Get().IsCreated()) {
            return {};
        }
        return runtime_terrain::TerrainSystem::Get().Materials().Layer(index).albedoPath;
    }

    void SetLayerMaterialPath(int index, std::string_view path) override {
        if (index < 0 || index >= m_LayerCount) {
            return;
        }
        if (!runtime_terrain::TerrainSystem::Get().IsCreated()) {
            return;
        }
        runtime_terrain::TerrainSystem::Get().Materials().Layer(index).albedoPath = std::string(path);
    }

    [[nodiscard]] bool AddLayer(std::string_view name) override {
        if (m_LayerCount >= runtime_terrain::kMaxPaintLayers) {
            return false;
        }
        const int index = m_LayerCount++;
        if (runtime_terrain::TerrainSystem::Get().IsCreated()) {
            auto& layer = runtime_terrain::TerrainSystem::Get().Materials().Layer(index);
            layer.name = name.empty() ? ("Layer " + std::to_string(index)) : std::string(name);
        }
        m_PaintLayer = index;
        return true;
    }

    [[nodiscard]] bool RemoveLayer(int index) override {
        if (index < 0 || index >= m_LayerCount || m_LayerCount <= 1) {
            return false;
        }
        if (runtime_terrain::TerrainSystem::Get().IsCreated()) {
            auto& mats = runtime_terrain::TerrainSystem::Get().Materials();
            for (int i = index; i < m_LayerCount - 1; ++i) {
                mats.Layer(i) = mats.Layer(i + 1);
            }
            mats.Layer(m_LayerCount - 1) = runtime_terrain::TerrainLayerDesc{};
        }
        --m_LayerCount;
        m_PaintLayer = std::clamp(m_PaintLayer, 0, m_LayerCount - 1);
        return true;
    }

    [[nodiscard]] int ActivePaintLayer() const noexcept override { return m_PaintLayer; }

    [[nodiscard]] bool ApplyBrushAtWorld(float worldX, float worldZ) override {
        if (!EnsureLandscape()) {
            return false;
        }
        auto& brushes = runtime_terrain::TerrainSystem::Get().Brushes();
        auto settings = brushes.Settings();
        const auto originalOp = settings.op;
        if (m_BrushUi.invert) {
            if (settings.op == runtime_terrain::TerrainBrushOp::Raise) {
                settings.op = runtime_terrain::TerrainBrushOp::Lower;
            } else if (settings.op == runtime_terrain::TerrainBrushOp::Lower) {
                settings.op = runtime_terrain::TerrainBrushOp::Raise;
            }
            brushes.SetSettings(settings);
        }

        bool changed = false;
        if (!m_StrokeActive) {
            if (!BeginBrushStroke()) {
                if (m_BrushUi.invert) {
                    settings.op = originalOp;
                    brushes.SetSettings(settings);
                }
                return false;
            }
            changed = runtime_terrain::TerrainSystem::Get().ApplyBrushWorld(worldX, worldZ);
            (void)EndBrushStroke();
            if (changed) {
                TerrainEditorDiagnostics::Get().OnBrushSample();
            }
        } else {
            changed = runtime_terrain::TerrainSystem::Get().ApplyBrushWorld(worldX, worldZ);
            if (changed) {
                TerrainEditorDiagnostics::Get().OnBrushSample();
            }
        }

        if (m_BrushUi.invert) {
            settings = brushes.Settings();
            settings.op = originalOp;
            brushes.SetSettings(settings);
        }
        return changed;
    }

    void UpdateBrushPreview(float worldX, float worldY, float worldZ) override {
        m_BrushPreview.worldX = worldX;
        m_BrushPreview.worldY = worldY;
        m_BrushPreview.worldZ = worldZ;
        m_BrushPreview.visible = m_BrushUi.showPreview;
        const auto& s = BrushSettings();
        m_BrushPreview.radiusWorld = s.radius;
        m_BrushPreview.strength = s.strength;
        m_BrushPreview.falloff = s.falloff;
        m_BrushPreview.op = s.op;
    }

    void SetPaintLayer(int layerIndex) override {
        m_PaintLayer = std::clamp(layerIndex, 0, std::max(0, m_LayerCount - 1));
        auto settings = runtime_terrain::TerrainSystem::Get().Brushes().Settings();
        settings.paintLayer = m_PaintLayer;
        runtime_terrain::TerrainSystem::Get().Brushes().SetSettings(settings);
    }

    [[nodiscard]] bool PaintLayerAtWorld(float worldX, float worldZ) override {
        if (!EnsureLandscape()) {
            return false;
        }
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        const auto& info = terrain.Info();
        const float localX = worldX - info.worldOrigin.x;
        const float localZ = worldZ - info.worldOrigin.z;
        const int sx = static_cast<int>(
            (info.worldSizeX > 1e-6f)
                ? (localX / info.worldSizeX) * static_cast<float>(info.resolutionX - 1)
                : 0.f);
        const int sz = static_cast<int>(
            (info.worldSizeY > 1e-6f)
                ? (localZ / info.worldSizeY) * static_cast<float>(info.resolutionY - 1)
                : 0.f);
        const auto& brush = terrain.Brushes().Settings();
        terrain.Materials().PaintWeight(sx, sz, m_PaintLayer, brush.strength, brush.radius);
        return true;
    }

    void SelectTerrainActor() override {
        const auto id = LandscapeEntityId();
        if (!id || !m_Viewport) {
            return;
        }
        m_Viewport->Selection().Set(ViewportObjectId{id});
        m_Viewport->Context().NotifySelectionChanged();
        m_Viewport->SelectionContext().SetPrimaryTerrain(ViewportObjectId{id});
    }

    [[nodiscard]] std::uint64_t LandscapeEntityId() const noexcept override {
        return runtime_terrain::TerrainSystem::Get().LandscapeEntityId();
    }

    [[nodiscard]] LandscapeBrushPreview& BrushPreview() noexcept override { return m_BrushPreview; }

    void Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ) override {
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            return;
        }
        terrain.Tick(deltaSeconds, we::math::Vec3(cameraX, cameraY, cameraZ), nullptr);
        terrain.Renderer().SyncChunks(terrain.Chunks());
    }

    [[nodiscard]] runtime_terrain::TerrainCreationReport VerifyRenderableLandscape(
        const char* stage) const
    {
        runtime_terrain::TerrainCreationReport report{};
        auto& terrain = runtime_terrain::TerrainSystem::Get();
        if (!terrain.IsCreated()) {
            report.failedStage = stage ? stage : "Verify";
            report.reason = "terrain_not_created";
            return report;
        }
        if (terrain.Heightmap().Empty()) {
            report.failedStage = "GenerateInitialTerrainData";
            report.reason = "heightfield_empty";
            return report;
        }
        if (terrain.Chunks().Chunks().empty()) {
            report.failedStage = "BuildTerrainChunks";
            report.reason = "no_chunks";
            return report;
        }
        std::uint32_t verts = 0;
        std::uint32_t tris = 0;
        std::uint32_t visible = 0;
        for (const auto& chunk : terrain.Chunks().Chunks()) {
            if (chunk.visible) {
                ++visible;
            }
            verts += static_cast<std::uint32_t>(chunk.mesh.positions.size());
            tris += static_cast<std::uint32_t>(chunk.mesh.indices.size() / 3);
            if (chunk.mesh.positions.empty() || chunk.mesh.indices.empty()) {
                report.failedStage = "GenerateMeshGeometry";
                report.reason = "chunk_mesh_empty";
                return report;
            }
            const auto& b = chunk.bounds;
            if (!(b.min.x <= b.max.x && b.min.y <= b.max.y && b.min.z <= b.max.z)) {
                report.failedStage = "GenerateMeshGeometry";
                report.reason = "invalid_bounds";
                return report;
            }
        }
        terrain.Renderer().SyncChunks(terrain.Chunks());
        report.render = terrain.Renderer().LastStats();
        report.render.chunksCreated = static_cast<std::uint32_t>(terrain.Chunks().Chunks().size());
        report.render.chunksVisible = visible;
        report.render.chunksLoaded = visible;
        report.render.vertices = verts;
        report.render.triangles = tris;
        report.render.materialAssigned =
            terrain.Info().materialSlot0.empty() ? "DefaultTerrain" : terrain.Info().materialSlot0;
        if (verts == 0 || tris == 0) {
            report.failedStage = "GenerateMeshGeometry";
            report.reason = "zero_geometry";
            return report;
        }
        if (terrain.Renderer().HasDevice()) {
            if (!terrain.Renderer().IsReady()) {
                report.failedStage = "CreateRenderProxy";
                report.reason = "terrain_renderer_not_ready";
                return report;
            }
            if (report.render.gpuBuffers == 0) {
                report.failedStage = "CreateGpuBuffers";
                report.reason = "no_gpu_buffers";
                return report;
            }
            report.render.renderProxyReady = true;
        }
        report.success = true;
        return report;
    }

    void LogCreationReport(const runtime_terrain::TerrainCreationReport& report) const {
        std::ostringstream oss;
        oss << "[TerrainEditor] Landscape pipeline diagnostics:"
            << " success=" << (report.success ? 1 : 0)
            << " chunks=" << report.render.chunksCreated
            << " visible=" << report.render.chunksVisible
            << " loaded=" << report.render.chunksLoaded
            << " verts=" << report.render.vertices
            << " tris=" << report.render.triangles
            << " gpuBuffers=" << report.render.gpuBuffers
            << " proxy=" << (report.render.renderProxyReady ? 1 : 0)
            << " pipeline=" << (report.render.pipelineReady ? 1 : 0)
            << " material=" << report.render.materialAssigned
            << " draws=" << report.render.drawCalls
            << " bounds=(" << report.render.bounds.min.x << "," << report.render.bounds.min.y
            << "," << report.render.bounds.min.z << ")-(" << report.render.bounds.max.x << ","
            << report.render.bounds.max.y << "," << report.render.bounds.max.z << ")";
        if (!report.success) {
            oss << " failedStage=" << report.failedStage << " reason=" << report.reason;
            HE_ERROR(oss.str());
        } else {
            HE_INFO(oss.str());
        }
    }

    void FrameLandscapeInViewport() {
        if (!m_Viewport) {
            return;
        }
        SelectTerrainActor();
        auto* cam = m_Viewport->Context().EditorCamera();
        if (!cam) {
            return;
        }
        const auto& info = runtime_terrain::TerrainSystem::Get().Info();
        const float midY = info.heightOffset
            + info.initialElevation * info.heightScale * info.worldScale.y;
        const we::math::Vec3 center = info.worldOrigin
            + we::math::Vec3(info.worldSizeX * 0.5f, midY, info.worldSizeY * 0.5f);
        const float distance = std::max(info.worldSizeX, info.worldSizeY) * 0.85f;
        cam->Focus(center, distance);
        const ViewportObjectId id{LandscapeEntityId()};
        if (id.value != 0) {
            m_Viewport->Camera().FrameSelection(std::span<const ViewportObjectId>(&id, 1));
            // Re-apply distance after FrameSelection (which uses Focus@8m).
            cam->Focus(center, distance);
        }
    }

    void InstallViewportMode() override {
        std::lock_guard lock(m_InstallMutex);
        if (m_Installed) {
            return;
        }

        ViewportModeDescriptor desc;
        desc.id = "Landscape";
        desc.displayName = "Landscape";
        desc.iconName = "mountain";
        desc.description = "Terrain sculpt, paint, and streaming";
        desc.builtinId = ViewportModeId::Landscape;
        desc.sortOrder = 10;

        GetViewportModeRegistry().Unregister("Landscape");
        GetViewportModeRegistry().RegisterFactory(std::move(desc), [this]() {
            return std::unique_ptr<IViewportMode>(std::make_unique<LandscapeMode>(*this));
        });

        if (m_Viewport) {
            m_Viewport->RegisterTool(std::make_unique<LandscapeSculptTool>(
                *this, ViewportToolId::LandscapeSculpt, "Sculpt",
                runtime_terrain::TerrainBrushOp::Raise));
            m_Viewport->RegisterTool(std::make_unique<LandscapeSculptTool>(
                *this, ViewportToolId::LandscapeSmooth, "Smooth",
                runtime_terrain::TerrainBrushOp::Smooth));
            m_Viewport->RegisterTool(std::make_unique<LandscapeSculptTool>(
                *this, ViewportToolId::LandscapeFlatten, "Flatten",
                runtime_terrain::TerrainBrushOp::Flatten));
            m_Viewport->RegisterTool(std::make_unique<LandscapeSculptTool>(
                *this, ViewportToolId::LandscapeNoise, "Noise",
                runtime_terrain::TerrainBrushOp::Noise));
            m_Viewport->RegisterTool(std::make_unique<LandscapeSculptTool>(
                *this, ViewportToolId::LandscapePaint, "Paint Layer",
                runtime_terrain::TerrainBrushOp::Raise));

            m_Viewport->Workspace().RegisterOverlay(
                std::make_unique<LandscapeBrushOverlay>(m_BrushPreview));
            m_Viewport->Workspace().RegisterRenderExtension(
                std::make_unique<LandscapeRenderExtension>(m_BrushPreview));

            (void)m_Viewport->Modes().LoadMode("Landscape");
        }

        m_Installed = true;
        HE_INFO("[TerrainEditor] Landscape viewport mode installed.");
    }

private:
    LandscapeEditorImpl() = default;

    void WireSessionUndo() {
        if (!m_TerrainRuntime || !m_Undo) {
            return;
        }
        m_TerrainRuntime->Session().SetTransactionRecorder(
            [this](std::string_view label, std::function<bool()> undoFn, std::function<bool()> redoFn) {
                return m_Undo->Manager().RecordCustom(
                    std::string(label),
                    undo::TransactionKind::Generic,
                    "terrain.edit",
                    std::move(undoFn),
                    std::move(redoFn));
            });
    }

    scene::Scene* m_Scene = nullptr;
    undo::IUndoRuntime* m_Undo = nullptr;
    IViewportEditor* m_Viewport = nullptr;
    runtime_terrain::ITerrainRuntime* m_TerrainRuntime = nullptr;
    LandscapeCreationWizard m_Wizard;
    NewLandscapeDialogState m_Dialog{};
    LandscapeBrushPreview m_BrushPreview{};
    LandscapeBrushUiState m_BrushUi{};
    std::vector<std::uint16_t> m_StrokeBefore;
    bool m_StrokeActive = false;
    int m_PaintLayer = 0;
    int m_LayerCount = 1;
    bool m_Installed = false;
    std::mutex m_InstallMutex;
};

} // namespace

ILandscapeEditor& GetLandscapeEditor() noexcept {
    return LandscapeEditorImpl::Get();
}

TerrainEditorService& TerrainEditorService::Get() {
    static TerrainEditorService instance;
    return instance;
}

void TerrainEditorService::BindScene(scene::Scene* scene) {
    GetLandscapeEditor().BindScene(scene);
}

void TerrainEditorService::BindUndo(undo::IUndoRuntime* undo) {
    GetLandscapeEditor().BindUndo(undo);
}

void TerrainEditorService::BindViewport(viewportedit::IViewportEditor* viewport) {
    GetLandscapeEditor().BindViewport(viewport);
}

bool TerrainEditorService::EnsureLandscape() {
    return GetLandscapeEditor().EnsureLandscape();
}

bool TerrainEditorService::GenerateDefaultLandscape() {
    return GetLandscapeEditor().GenerateDefaultLandscape();
}

bool TerrainEditorService::ImportHeightmap(const std::filesystem::path& path) {
    return GetLandscapeEditor().ImportHeightmap(path);
}

bool TerrainEditorService::ExportHeightmap(const std::filesystem::path& path) {
    return GetLandscapeEditor().ExportHeightmap(path);
}

void TerrainEditorService::SetBrushOp(runtime_terrain::TerrainBrushOp op) {
    GetLandscapeEditor().SetBrushOp(op);
}

void TerrainEditorService::SetBrushRadius(float radius) {
    GetLandscapeEditor().SetBrushRadius(radius);
}

void TerrainEditorService::SetBrushStrength(float strength) {
    GetLandscapeEditor().SetBrushStrength(strength);
}

bool TerrainEditorService::ApplyBrushAtWorld(float worldX, float worldZ) {
    return GetLandscapeEditor().ApplyBrushAtWorld(worldX, worldZ);
}

int TerrainEditorService::SpawnFoliageInDefaultRegion() {
    if (!EnsureLandscape()) {
        return 0;
    }
    auto& terrain = runtime_terrain::TerrainSystem::Get();
    runtime_terrain::FoliageSpawnParams params{};
    params.densityPerSquareMeter = 0.02f;
    params.maxSlopeDegrees = 30.0f;
    const float half = terrain.Info().worldSizeX * 0.35f;
    return terrain.Foliage().Spawn(
        terrain.Collision(),
        terrain.Info(),
        &terrain.Materials(),
        params,
        we::math::Vec2(-half, -half),
        we::math::Vec2(half, half));
}

void TerrainEditorService::Tick(float deltaSeconds, float cameraX, float cameraY, float cameraZ) {
    GetLandscapeEditor().Tick(deltaSeconds, cameraX, cameraY, cameraZ);
}

void TerrainEditorService::InstallViewportMode() {
    GetLandscapeEditor().InstallViewportMode();
}

ILandscapeEditor& TerrainEditorService::Landscape() noexcept {
    return GetLandscapeEditor();
}

} // namespace we::editor::terrain
