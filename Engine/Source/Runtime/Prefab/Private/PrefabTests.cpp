#include "Prefab/PrefabTests.h"
#include "Prefab/Prefab.h"
#include "Scene/Scene.h"
#include "Serialization/Delta.h"

#include <filesystem>
#include <sstream>

namespace we::runtime::prefab {
namespace {

void AddCase(PrefabTestReport& report, std::string name, bool passed, std::string message) {
    PrefabTestCaseResult result;
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

PrefabTestReport RunPrefabRuntimeTests() {
    PrefabTestReport report{};
    PrefabDiagnostics::Get().Reset();

    scene::Scene scene;
    scene.CreateEntity("CubeA", scene::EntityType::Cube);
    const auto& entities = scene.GetEntities();
    AddCase(report, "Scene has entity", !entities.empty(), entities.empty() ? "empty" : "ok");
    if (entities.empty()) {
        report.success = false;
        report.summary = "Scene setup failed";
        return report;
    }
    const std::uint64_t rootId = entities.front().Id;

    PrefabDependencies deps;
    deps.scene = &scene;
    auto runtime = CreatePrefabRuntime(deps);
    AddCase(report, "CreatePrefabRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create PrefabRuntime";
        return report;
    }

    auto& mgr = runtime->Manager();
    const auto tempPath =
        (std::filesystem::temp_directory_path() / "we_prefab_test.weprefab").string();

    PrefabCommandContext createCtx;
    createCtx.rootEntityId = rootId;
    createCtx.name = "CubePrefab";
    createCtx.path = tempPath;
    const bool created = mgr.Commands().Execute(PrefabCommandId::CreateFromSelection, createCtx);
    AddCase(report, "CreateFromSelection", created, created ? "ok" : "fail");

    const auto guids = mgr.Registry().ListAll();
    AddCase(report, "Registry has asset", !guids.empty(), guids.empty() ? "empty" : "ok");

    PrefabGuid guid{};
    if (!guids.empty()) {
        guid = guids.front();
    }

    auto loaded = mgr.Serializer().LoadFromFile(tempPath);
    AddCase(report, "Load .weprefab", loaded != nullptr, loaded ? "ok" : "fail");
    if (loaded) {
        AddCase(report, "Roundtrip guid", loaded->GetGuid() == guid, "guid");
        AddCase(report, "Roundtrip name", loaded->GetName() == "CubePrefab", std::string(loaded->GetName()));
    }

    PrefabSpawnParams spawn;
    spawn.prefab = guid;
    spawn.position = we::math::Vec3{1.f, 2.f, 3.f};
    spawn.instanceName = "CubeInstance";
    const auto instanceId = mgr.Spawner().Spawn(spawn);
    AddCase(report, "Spawn instance", instanceId.IsValid(), instanceId.IsValid() ? "ok" : "fail");

    auto* instance = mgr.Spawner().FindInstance(instanceId);
    AddCase(report, "Find instance", instance != nullptr, instance ? "ok" : "null");

    serialization::BinaryDiff overrideDiff{};
    overrideDiff.identical = false;
    serialization::BinaryDiffEntry overrideEntry;
    overrideEntry.path = "Position";
    overrideEntry.after = {1, 2, 3};
    overrideDiff.entries.push_back(overrideEntry);
    const bool recorded = mgr.RecordPropertyOverride(instanceId, "Position", overrideDiff);
    AddCase(report, "Record override", recorded, recorded ? "ok" : "fail");

    const bool broken = mgr.BreakLink(instanceId);
    AddCase(report, "Break link", broken, broken ? "ok" : "fail");
    const bool restored = mgr.RestoreLink(instanceId, guid);
    AddCase(report, "Restore link", restored, restored ? "ok" : "fail");

    const auto issues = mgr.Validator().ValidateAsset(guid);
    AddCase(report, "Validate asset", issues.empty(), issues.empty() ? "ok" : issues.front().message);

    PrefabCommandContext dupCtx;
    dupCtx.asset = guid;
    dupCtx.name = "CubePrefab_Copy";
    dupCtx.path = (std::filesystem::temp_directory_path() / "we_prefab_copy.weprefab").string();
    const bool duplicated = mgr.Commands().Execute(PrefabCommandId::DuplicateAsset, dupCtx);
    AddCase(report, "Duplicate asset", duplicated, duplicated ? "ok" : "fail");

    const bool despawned = mgr.Spawner().Despawn(instanceId);
    AddCase(report, "Despawn", despawned, despawned ? "ok" : "fail");

    runtime->Shutdown();
    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
    std::filesystem::remove(dupCtx.path, ec);

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "Prefab tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::prefab
