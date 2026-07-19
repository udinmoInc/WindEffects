#include "WorldOutliner/WorldOutlinerTests.h"
#include "WorldOutliner/WorldOutliner.h"
#include "Undo/Undo.h"
#include "Scene/Scene.h"

#include <cmath>
#include <sstream>

namespace we::editor::outliner {
namespace {

void AddCase(WorldOutlinerTestReport& report, std::string name, bool passed, std::string message) {
    WorldOutlinerTestCaseResult result;
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

WorldOutlinerTestReport RunWorldOutlinerRuntimeTests() {
    WorldOutlinerTestReport report{};
    WorldOutlinerDiagnostics::Get().Reset();

    auto undoRuntime = undo::CreateUndoRuntime({});
    auto scene = std::make_shared<scene::Scene>();
    scene->CreateEntity("RootFolder", scene::EntityType::EmptyActor);
    scene->CreateEntity("CubeA", scene::EntityType::Cube);
    scene->CreateEntity("CubeB", scene::EntityType::Cube);
    auto& entities = scene->GetEntities();
    const auto rootId = entities[0].Id;
    entities[1].ParentId = rootId;
    entities[2].ParentId = rootId;

    WorldOutlinerDependencies deps;
    deps.undo = undoRuntime.get();
    deps.scene = scene.get();

    auto runtime = CreateWorldOutlinerRuntime(deps);
    AddCase(report, "CreateWorldOutlinerRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create WorldOutlinerRuntime";
        return report;
    }

    auto& outliner = runtime->Outliner();
    outliner.RequestRebuild();
    outliner.Tick(0.f);

    AddCase(report, "Model has rows", !outliner.Model().GetVisibleRows().empty(), "visible");

    const OutlinerNodeId cubeA{entities[1].Id};
    outliner.Selection().Set(cubeA);
    outliner.SyncSelectionToScene();
    AddCase(report, "Selection sync to Scene", scene->GetSelectedEntityId() == cubeA.value, "scene");

    const bool renamed = outliner.Commands().Rename(cubeA, "CubeRenamed");
    AddCase(report, "Rename", renamed && entities[1].Name == "CubeRenamed", "name");
    AddCase(report, "Rename undoable", undoRuntime->Manager().CanUndo(), "canUndo");
    (void)undoRuntime->Manager().Undo();
    AddCase(report, "Undo rename", entities[1].Name == "CubeA", "restored");

    const bool reparented = outliner.Commands().Reparent(OutlinerNodeId{entities[2].Id}, cubeA);
    AddCase(report, "Reparent", reparented && entities[2].ParentId == cubeA.value, "parent");

    OutlinerFilterState filter;
    filter.searchQuery = "Cube";
    outliner.SetFilterState(filter);
    outliner.Tick(0.f);
    AddCase(report, "Search filter", !outliner.Model().GetVisibleRows().empty(), "filtered");

    outliner.SaveFilterPreset("cubes");
    filter.searchQuery.clear();
    outliner.SetFilterState(filter);
    AddCase(report, "Load preset", outliner.LoadFilterPreset("cubes"), "preset");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "WorldOutliner tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::outliner
