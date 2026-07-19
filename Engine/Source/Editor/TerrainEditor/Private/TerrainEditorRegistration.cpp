#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "TerrainEditor/TerrainEditor.h"
#include "TerrainEditor/Widgets/LandscapeWorkspacePanel.h"
#include "Terrain/TerrainTypes.h"
#include "ViewportEdit/ViewportEditSession.h"

namespace we::programs::editor {

using ::we::editor::terrain::TerrainEditorService;
using ::we::editor::terrain::GetLandscapeEditor;
using ::we::editor::terrain::LandscapeWorkspacePanel;
using we::runtime::terrain::TerrainBrushOp;

namespace {

void ActivateLandscapeViewportMode() {
    TerrainEditorService::Get().InstallViewportMode();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveMode("Landscape");
    }
}

struct LandscapeModeBridge {
    LandscapeModeBridge() {
        we::editor::shell::EditorModeController::Get().AddModeChangedListener(
            [](const std::string& modeId) {
                if (modeId == "Landscape") {
                    ActivateLandscapeViewportMode();
                } else if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
                    if (editor->GetActiveModeKey() == "Landscape") {
                        editor->SetActiveMode("Select");
                    }
                }
            });
    }
};

static LandscapeModeBridge g_LandscapeModeBridge;

void ConfigureLandscapeModePanel() {
    we::editor::toolspanel::EditorToolMode mode;
    if (const auto* existing = we::editor::toolspanel::EditorToolsRegistry::Get().FindMode("Landscape")) {
        mode = *existing;
    } else {
        mode.id = "Landscape";
        mode.label = "Landscape";
        mode.iconName = "grid";
        mode.sortOrder = 30;
        mode.opensToolDrawerByDefault = true;
    }

    mode.customContent = [](const we::editor::toolspanel::EditorToolMode&, const std::string&) {
        return std::make_shared<LandscapeWorkspacePanel>(&GetLandscapeEditor());
    };
    we::editor::toolspanel::EditorToolsRegistry::Get().RegisterMode(std::move(mode));
}

struct LandscapeWorkspaceRegistration {
    LandscapeWorkspaceRegistration() {
        ConfigureLandscapeModePanel();
    }
};

static LandscapeWorkspaceRegistration g_LandscapeWorkspaceRegistration;

} // namespace

// Keep tool registrations for shortcuts / search; primary UX is the workspace panel.
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptRaise, "Raise", "plus", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Raise);
    TerrainEditorService::Get().EnsureLandscape();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveTool(we::editor::viewportedit::ViewportToolId::LandscapeSculpt);
    }
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptLower, "Lower", "minus", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Lower);
    TerrainEditorService::Get().EnsureLandscape();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveTool(we::editor::viewportedit::ViewportToolId::LandscapeSculpt);
    }
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptSmooth, "Smooth", "refresh", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Smooth);
    TerrainEditorService::Get().EnsureLandscape();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveTool(we::editor::viewportedit::ViewportToolId::LandscapeSmooth);
    }
})
REGISTER_EDITOR_TOOL(LandscapeSculpt, SculptFlatten, "Flatten", "plane", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Flatten);
    TerrainEditorService::Get().EnsureLandscape();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveTool(we::editor::viewportedit::ViewportToolId::LandscapeFlatten);
    }
})

REGISTER_EDITOR_TOOL_CATEGORY(Landscape, LandscapeErosion, "Erosion", "layers", 20)
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptNoise, "Noise", "star", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::Noise);
    TerrainEditorService::Get().EnsureLandscape();
    if (auto* editor = we::editor::viewportedit::ViewportEditSession::Editor()) {
        editor->SetActiveTool(we::editor::viewportedit::ViewportToolId::LandscapeNoise);
    }
})
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptHydraulic, "Hydraulic Erosion", "refresh", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::HydraulicErosion);
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(LandscapeErosion, SculptThermal, "Thermal Erosion", "snap", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().SetBrushOp(TerrainBrushOp::ThermalErosion);
    TerrainEditorService::Get().EnsureLandscape();
})

REGISTER_EDITOR_TOOL(TerrainTools, TerrainGenerate, "Generate Terrain", "grid", "", []() {
    ActivateLandscapeViewportMode();
    auto& landscape = GetLandscapeEditor();
    landscape.Wizard().Reset();
    landscape.Dialog() = landscape.Wizard().State();
    landscape.Dialog().generateProcedural = true;
    landscape.Wizard().State() = landscape.Dialog();
    (void)landscape.CreateFromDialog();
})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainImport, "Import Heightmap", "open", "", []() {
    ActivateLandscapeViewportMode();
    TerrainEditorService::Get().EnsureLandscape();
})
REGISTER_EDITOR_TOOL(TerrainTools, TerrainNewLandscape, "New Landscape", "plus", "", []() {
    ActivateLandscapeViewportMode();
    auto& landscape = GetLandscapeEditor();
    landscape.Wizard().Reset();
    (void)landscape.CreateFromDialog();
})

REGISTER_EDITOR_TOOL(FoliagePaint, FoliagePaintTool, "Paint Foliage", "layers", "Shift+4", []() {
    TerrainEditorService::Get().SpawnFoliageInDefaultRegion();
})

} // namespace we::programs::editor
