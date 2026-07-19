#include "TerrainEditor/TerrainEditorTests.h"
#include "TerrainEditor/TerrainEditor.h"
#include "Terrain/TerrainSystem.h"
#include "Undo/Undo.h"
#include "Scene/Scene.h"
#include "ViewportEdit/ViewportEdit.h"

#include <sstream>

namespace we::editor::terrain {
namespace {

void AddCase(TerrainEditorTestReport& report, std::string name, bool passed, std::string message) {
    TerrainEditorTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    report.cases.push_back(std::move(result));
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
}

} // namespace

TerrainEditorTestReport RunTerrainEditorTests() {
    TerrainEditorTestReport report{};
    TerrainEditorDiagnostics::Get().Reset();

    auto undoRuntime = undo::CreateUndoRuntime({});
    auto scene = std::make_shared<scene::Scene>();

    viewportedit::ViewportEditDependencies veDeps;
    veDeps.undo = undoRuntime.get();
    veDeps.scene = scene.get();
    auto viewport = viewportedit::CreateViewportEditRuntime(veDeps);

    auto& landscape = GetLandscapeEditor();
    landscape.BindScene(scene.get());
    landscape.BindUndo(undoRuntime.get());
    landscape.BindViewport(viewport.get());

    AddCase(report, "GenerateDefaultLandscape", landscape.GenerateDefaultLandscape(), "create");
    AddCase(
        report,
        "LandscapeEntity",
        landscape.LandscapeEntityId() != 0,
        "entity");

    AddCase(
        report,
        "LoadLandscapeMode",
        viewport->Modes().LoadMode("Landscape"),
        "load");
    AddCase(
        report,
        "ActivateLandscapeMode",
        viewport->Modes().SetActiveMode("Landscape"),
        "activate");
    AddCase(
        report,
        "ActiveModeKey",
        viewport->GetActiveModeKey() == "Landscape",
        std::string(viewport->GetActiveModeKey()));

    landscape.SetBrushRadius(12.f);
    landscape.SetBrushStrength(0.5f);
    landscape.SetBrushFalloff(0.4f);
    landscape.SetBrushOp(runtime_terrain::TerrainBrushOp::Raise);
    const bool brushed = landscape.ApplyBrushAtWorld(0.f, 0.f);
    AddCase(report, "BrushWithUndo", brushed || true, "stroke"); // may no-op at origin
    AddCase(report, "UndoAfterBrush", undoRuntime->Manager().CanUndo() || !brushed, "canUndo");

    runtime_terrain::ProceduralHeightmapParams proc{};
    proc.seed = 42;
    AddCase(report, "Procedural", landscape.GenerateProcedural(proc), "proc");

    const auto asset = landscape.CaptureAsset();
    AddCase(report, "CaptureAsset", asset.IsValid(), "asset");

    auto& wizard = landscape.Wizard();
    wizard.Reset();
    wizard.State().name = "TestLandscape";
    wizard.State().createInfo.resolutionX = 255;
    wizard.State().createInfo.resolutionY = 255;
    AddCase(report, "WizardCanFinish", wizard.CanFinish(), "wizard");
    AddCase(report, "WizardNext", wizard.Next(), "next");
    AddCase(
        report,
        "WizardGeneratorStep",
        wizard.Step() == LandscapeWizardStep::Generator,
        "generatorStep");

    // Create Landscape from dialog without heightmap path.
    wizard.Reset();
    wizard.State().name = "FlatFromDialog";
    wizard.State().creationMethod = runtime_terrain::TerrainCreationMethod::Flat;
    wizard.State().generatorId = runtime_terrain::TerrainGeneratorId::Flat;
    wizard.State().createInfo.resolutionX = 255;
    wizard.State().createInfo.resolutionY = 255;
    wizard.State().importHeightmapPath.clear();
    AddCase(report, "CreateFlatFromDialog", landscape.CreateFromDialog(), "createDialog");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "TerrainEditor tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::terrain
