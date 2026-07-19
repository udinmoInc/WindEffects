#include "ViewportEdit/ViewportEditTests.h"
#include "ViewportEdit/ViewportEdit.h"
#include "Undo/Undo.h"
#include "Scene/Scene.h"
#include "EditorCamera.h"

#include <sstream>

namespace we::editor::viewportedit {
namespace {

void AddCase(ViewportEditTestReport& report, std::string name, bool passed, std::string message) {
    ViewportEditTestCaseResult result;
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

ViewportEditTestReport RunViewportEditRuntimeTests() {
    ViewportEditTestReport report{};
    ViewportEditDiagnostics::Get().Reset();

    auto undoRuntime = undo::CreateUndoRuntime({});
    auto scene = std::make_shared<scene::Scene>();
    auto camera = std::make_shared<engine::EditorCamera>();
    camera->SetViewportSize(800.f, 600.f);

    scene->CreateEntity("CubeA", scene::EntityType::Cube);
    scene->CreateEntity("CubeB", scene::EntityType::Cube);
    auto& entities = scene->GetEntities();
    entities[0].Position = {0.f, 0.f, 0.f};
    entities[1].Position = {5.f, 0.f, 0.f};

    ViewportEditDependencies deps;
    deps.undo = undoRuntime.get();
    deps.scene = scene.get();
    deps.editorCamera = camera.get();

    auto editor = CreateViewportEditRuntime(deps);
    AddCase(report, "CreateViewportEditRuntime", editor != nullptr, editor ? "ok" : "null");
    if (!editor) {
        report.success = false;
        report.summary = "Failed to create ViewportEditRuntime";
        return report;
    }

    editor->SetViewportSize(800.f, 600.f);
    editor->SetActiveTool(ViewportToolId::Select);

    // Selection API
    const ViewportObjectId a{entities[0].Id};
    const ViewportObjectId b{entities[1].Id};
    editor->Selection().Set(a);
    AddCase(report, "Select single", editor->Selection().GetPrimary() == a, "primary");
    editor->SyncSelectionToScene();
    AddCase(report, "Sync to Scene", scene->GetSelectedEntityId() == a.value, "scene id");

    editor->Selection().Apply(SelectionOp::Add, b);
    AddCase(report, "Multi-select", editor->Selection().Count() == 2, "count");

    // Transform + Undo
    editor->SetActiveTool(ViewportToolId::Move);
    const float beforeX = entities[0].Position.x;
    editor->Manipulator().ApplyTranslation({1.f, 0.f, 0.f});
    AddCase(report, "Move applies", entities[0].Position.x > beforeX + 0.5f, "pos");
    AddCase(report, "Move records undo", undoRuntime->Manager().CanUndo(), "canUndo");
    (void)undoRuntime->Manager().Undo();
    AddCase(report, "Undo restores", std::abs(entities[0].Position.x - beforeX) < 1e-3f, "restored");

    // Snap
    SnapSettings snapSettings;
    snapSettings.gridEnabled = true;
    snapSettings.gridSize = 1.f;
    editor->Snap().SetSettings(snapSettings);
    const auto snapped = editor->Snap().SnapTranslation({1.4f, 2.6f, -0.4f});
    AddCase(
        report,
        "Grid snap",
        std::abs(snapped.x - 1.f) < 1e-3f && std::abs(snapped.y - 3.f) < 1e-3f
            && std::abs(snapped.z - 0.f) < 1e-3f,
        "snapped");

    // Hit test smoke (may miss depending on camera; still must not crash)
    const auto hit = editor->HitTester().Pick(400.f, 300.f, 800.f, 600.f);
    AddCase(report, "Pick smoke", true, hit.valid ? "hit" : "miss");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "ViewportEdit tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::viewportedit
