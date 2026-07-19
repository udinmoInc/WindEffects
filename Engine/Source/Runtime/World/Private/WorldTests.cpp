#include "World/WorldTests.h"
#include "World/World.h"

#include <cstring>
#include <sstream>

namespace we::runtime::world {
namespace {

void AddCase(WorldTestReport& report, std::string name, bool passed, std::string message) {
    WorldTestCaseResult result;
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

WorldTestReport RunWorldRuntimeTests() {
    WorldTestReport report{};
    WorldDiagnostics::Get().Reset();

    auto runtime = CreateWorldRuntime({});
    AddCase(report, "CreateWorldRuntime", runtime != nullptr, runtime ? "ok" : "null runtime");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create World Runtime";
        return report;
    }

    WorldCreateInfo info{};
    std::memcpy(info.descriptor.name, "TestWorld", 10);
    info.descriptor.persistent = true;
    info.createPersistentLevel = true;
    const WorldHandle worldHandle = runtime->Manager().CreateWorld(info);
    AddCase(report, "CreateWorld", worldHandle.IsValid(), "handle");

    IWorld* world = runtime->Manager().FindWorld(worldHandle);
    AddCase(report, "FindWorld", world != nullptr, world ? std::string(world->Name()) : "missing");
    if (!world) {
        report.success = false;
        report.summary = "World missing";
        return report;
    }

    AddCase(report, "PersistentLevel", world->PersistentLevel().IsValid(), "level");
    AddCase(report, "ActiveScene", world->ActiveScene() != nullptr, "scene");
    AddCase(report, "ActiveEcsWorld", world->ActiveEcsWorld() != nullptr, "ecs");

    ActorSpawnParams spawn{};
    std::memcpy(spawn.name, "Hero", 5);
    spawn.localPosition[0] = 1.f;
    spawn.localPosition[1] = 2.f;
    spawn.localPosition[2] = 3.f;
    const ActorHandle hero = world->Actors().Spawn(spawn);
    AddCase(report, "SpawnActor", hero.IsValid(), "hero");

    ActorSpawnParams childSpawn{};
    std::memcpy(childSpawn.name, "Child", 6);
    childSpawn.parent = hero;
    const ActorHandle child = world->Actors().Spawn(childSpawn);
    AddCase(report, "SpawnChild", child.IsValid(), "child");
    AddCase(report, "HierarchyParent", world->Hierarchy().GetParent(child) == hero, "parent link");

    const TagId enemy = world->Tags().RegisterTag("Enemy");
    world->Tags().AddTag(hero, enemy);
    AddCase(report, "TagSystem", world->Tags().HasTag(hero, enemy), "Enemy");

    const LayerId gameplay = world->Layers().RegisterLayer("Gameplay");
    world->Layers().SetLayer(hero, gameplay);
    AddCase(report, "LayerSystem", world->Layers().GetLayer(hero) == gameplay, "Gameplay");

    world->BeginPlay();
    AddCase(report, "BeginPlay", world->Actors().TryGet(hero) && world->Actors().TryGet(hero)->HasBegunPlay(), "playing");

    WorldTickParams tick{};
    tick.deltaSeconds = 1.f / 60.f;
    tick.fixedDeltaSeconds = 1.f / 60.f;
    world->Tick(tick);
    AddCase(report, "Tick", world->Context().FrameIndex() > 0, "frame");

    auto named = world->Queries().QueryByName("Hero");
    AddCase(report, "QueryByName", named.size() == 1, "count");

    world->Spatial().Rebuild();
    Sphere3f sphere{};
    sphere.center = {1.f, 2.f, 3.f};
    sphere.radius = 5.f;
    auto spatial = world->Spatial().OverlapSphere(sphere);
    AddCase(report, "SpatialQuery", !spatial.empty(), "hits");

    world->Transforms().SetLocalPosition(hero, Vec3f{10.f, 0.f, 0.f});
    const Vec3f pos = world->Transforms().GetLocalPosition(hero);
    AddCase(report, "Transform", pos.x == 10.f, "x=10");

    const auto saveBytes = world->Persistence().SaveWorld({});
    AddCase(report, "SaveWorld", !saveBytes.empty(), "bytes");
    AddCase(report, "LoadWorld", world->Persistence().LoadWorld(saveBytes), "roundtrip");

    PrefabInstanceParams prefab{};
    std::memcpy(prefab.spawn.name, "PrefabActor", 12);
    const ActorHandle prefabActor = world->Prefabs().Instantiate(prefab);
    AddCase(report, "PrefabInstantiate", prefabActor.IsValid(), "prefab");

    world->OriginRebasing().SetEnabled(true);
    world->OriginRebasing().RequestRebase(Vec3f{100.f, 0.f, 0.f});
    world->OriginRebasing().ApplyPendingRebase();
    AddCase(report, "OriginRebase", world->OriginRebasing().CurrentOrigin().x == 100.f, "origin");

    WorldCreateInfo info2{};
    std::memcpy(info2.descriptor.name, "SecondWorld", 12);
    const WorldHandle world2 = runtime->Manager().CreateWorld(info2);
    AddCase(report, "MultiWorld", runtime->Manager().Worlds().size() >= 2, "count");

    LevelDescriptor streamLevel{};
    std::memcpy(streamLevel.name, "StreamA", 8);
    streamLevel.streamable = true;
    const LevelHandle streamed = world->CreateLevel(streamLevel);
    StreamRequest req{};
    req.level = streamed;
    AddCase(report, "StreamLoad", world->Streamer().LoadLevel(req), "load");
    AddCase(report, "StreamUnload", world->Streamer().UnloadLevel(streamed), "unload");

    AddCase(report, "DestroyActor", world->Actors().Destroy(child), "child gone");
    AddCase(report, "DestroyWorld2", runtime->Manager().DestroyWorld(world2), "second");

    runtime->Shutdown();
    AddCase(report, "Shutdown", true, "ok");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "WorldRuntimeTests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::world
