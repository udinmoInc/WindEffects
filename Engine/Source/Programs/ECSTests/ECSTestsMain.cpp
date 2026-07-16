#include "ECS/CommandBuffer.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/Prefab.h"
#include "ECS/Registry.h"
#include "ECS/RenderExtract.h"
#include "ECS/SharedComponentStore.h"
#include "ECS/System.h"
#include "ECS/World.h"
#include "Scene/Scene.h"

#include "Core/BuildPaths.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
struct ECSTestsBootstrap {
    ECSTestsBootstrap() { we::core::ConfigureModuleSearchPaths(); }
};
#pragma init_seg(lib)
ECSTestsBootstrap g_Bootstrap;
#endif

namespace {

using namespace we::runtime::ecs;
using we::runtime::scene::Scene;
using we::runtime::scene::EntityType;

int g_Failed = 0;
int g_Passed = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << (msg) << "\n"; \
            ++g_Failed; \
        } else { \
            ++g_Passed; \
        } \
    } while (0)

void TestCreateDestroy() {
    std::cout << "[test] CreateDestroy\n" << std::flush;
    World world;
    Entity a = world.CreateEntity("A");
    Entity b = world.CreateEntity("B");
    CHECK(world.Valid(a), "entity A valid");
    CHECK(world.Valid(b), "entity B valid");
    CHECK(world.Has<TransformComponent>(a), "default Transform");
    CHECK(world.Has<HierarchyComponent>(a), "default Hierarchy");
    world.DestroyEntity(a);
    CHECK(!world.Valid(a), "entity A destroyed");
    CHECK(world.Valid(b), "entity B still valid");
}

void TestArchetypeMigration() {
    std::cout << "[test] ArchetypeMigration\n";
    World world;
    Entity e = world.CreateEntity();
    CHECK(!world.Has<DirectionalLightComponent>(e), "no light initially");
    world.AddComponent<DirectionalLightComponent>(e, DirectionalLightComponent{});
    world.Get<DirectionalLightComponent>(e).intensity = 3.0f;
    CHECK(world.Has<DirectionalLightComponent>(e), "light after add");
    CHECK(std::fabs(world.Get<DirectionalLightComponent>(e).intensity - 3.0f) < 0.01f, "light value");
    world.AddComponent<CameraComponent>(e, CameraComponent{});
    CHECK(world.Has<CameraComponent>(e) && world.Has<DirectionalLightComponent>(e), "both after second add");
    world.RemoveComponent<DirectionalLightComponent>(e);
    CHECK(!world.Has<DirectionalLightComponent>(e), "light removed");
    CHECK(world.Has<CameraComponent>(e), "camera retained");
}

void TestCommandBuffer() {
    std::cout << "[test] CommandBuffer\n";
    World world;
    CommandBuffer cmd;
    Entity e = world.CreateEntity();
    DirectionalLightComponent light{};
    light.intensity = 2.5f;
    cmd.AddComponent<DirectionalLightComponent>(e, light);
    CHECK(!world.Has<DirectionalLightComponent>(e), "deferred before flush");
    cmd.Flush(world);
    CHECK(world.Has<DirectionalLightComponent>(e), "present after flush");
    CHECK(std::fabs(world.Get<DirectionalLightComponent>(e).intensity - 2.5f) < 0.01f, "cmd payload");
}

void TestHierarchyTransform() {
    std::cout << "[test] HierarchyTransform\n";
    Registry registry;
    SystemScheduler scheduler;
    RegisterDefaultSystems(scheduler);
    scheduler.OnCreate(registry);

    Entity root = registry.Create("root");
    Entity child = registry.Create("child");
    HierarchySystem hierarchy;
    hierarchy.SetParent(registry, child, root);

    TransformComponent& rt = registry.Get<TransformComponent>(root);
    rt.localPosition = { 10.0f, 0.0f, 0.0f };
    rt.dirty = true;
    TransformComponent& ct = registry.Get<TransformComponent>(child);
    ct.localPosition = { 5.0f, 0.0f, 0.0f };
    ct.dirty = true;

    scheduler.Update(registry, 0.016f);

    const TransformComponent& childAfter = registry.Get<TransformComponent>(child);
    CHECK(std::fabs(childAfter.worldMatrix[3][0] - 15.0f) < 0.01f, "child world x = 15");
    CHECK(registry.Get<HierarchyComponent>(child).depth == 1, "child depth 1");
    scheduler.OnDestroy(registry);
}

void TestSharedStore() {
    std::cout << "[test] SharedStore\n";
    SharedComponentStore& store = SharedComponentStore::Get();
    store.Clear();
    const std::uint32_t hash = SharedComponentStore::HashBytes("matA", 4);
    SharedComponentRef a = store.Acquire(1, hash, "matA", 4);
    SharedComponentRef b = store.Acquire(1, hash, "matA", 4);
    CHECK(a.index == b.index, "dedupe same payload");
    CHECK(store.GetRefCount(a) == 2, "refcount 2");
    store.Release(a);
    CHECK(store.GetRefCount(b) == 1, "refcount 1 after release");
    store.Release(b);
    CHECK(store.AliveCount() == 0, "freed when zero");
}

void TestRenderExtract() {
    std::cout << "[test] RenderExtract\n";
    Registry registry;
    SystemScheduler scheduler;
    RegisterDefaultSystems(scheduler);
    scheduler.OnCreate(registry);

    Entity e = registry.Create("mesh");
    registry.Replace(e, StaticMeshComponent{ 1, {} });
    registry.Replace(e, MaterialComponent{ 2, {}, {0.2f, 0.4f, 0.8f, 1.0f} });
    registry.Get<TransformComponent>(e).dirty = true;
    registry.Get<VisibilityComponent>(e).visible = true;

    scheduler.Update(registry, 0.016f);

    const ExtractedFrameData* frame = nullptr;
    for (const auto& system : scheduler.Systems()) {
        if (system && std::string(system->Name()) == "RenderExtractionSystem") {
            frame = &static_cast<RenderExtractionSystem*>(system.get())->FrameData();
            break;
        }
    }
    CHECK(frame != nullptr, "extract system present");
    CHECK(frame && frame->meshes.size() == 1, "one mesh extracted");
    CHECK(frame && frame->meshes[0].meshAssetId == 1, "mesh asset id");
    scheduler.OnDestroy(registry);
}

void TestSceneEcsAuthority() {
    std::cout << "[test] SceneEcsAuthority\n";
    Scene scene;
    scene.CreateEntity("CubeA", EntityType::Cube);
    scene.CreateEntity("DirLight", EntityType::DirectionalLight);
    CHECK(scene.Registry().LivingCount() >= 2, "ecs living count");
    CHECK(!scene.IsEmpty(), "scene not empty");
    scene.GetEntities()[0].Position = { 3.0f, 1.0f, 0.0f };
    scene.Update();
    const auto* t = scene.Registry().TryGet<TransformComponent>(
        Entity{ scene.GetEntities()[0].Id });
    CHECK(t && std::fabs(t->localPosition.x - 3.0f) < 0.01f, "view→ecs sync");
    CHECK(scene.GetExtractedFrame() != nullptr, "extract available");
    CHECK(scene.GetExtractedFrame()->meshes.size() >= 1, "scene mesh extract");
}

void TestSerializationHooks() {
    std::cout << "[test] SerializationHooks\n";
    World world;
    Entity e = world.CreateEntity("ser");
    TagComponent tag{};
    tag.mask = 0xABC;
    const char* label = "tag";
    for (std::size_t i = 0; i < sizeof(tag.label) - 1 && label[i]; ++i) {
        tag.label[i] = label[i];
    }
    world.AddComponent<TagComponent>(e, tag);
    PrefabAsset asset = PrefabRegistry::Get().CaptureFromWorld(world, e, "p");
    CHECK(!asset.entities.empty(), "prefab capture");
    const PrefabId id = PrefabRegistry::Get().Register("p", asset);
    Entity inst = InstantiatePrefab(world, id);
    CHECK(world.Valid(inst), "prefab instantiate");
}

void TestMillionEntities() {
    std::cout << "[test] StressEntities\n";
    const char* envCount = std::getenv("WE_ECS_STRESS_COUNT");
    // Default 100k for CI-friendly runtime; set WE_ECS_STRESS_COUNT=1000000 for full AAA scale.
    const std::size_t count = envCount ? static_cast<std::size_t>(std::atoll(envCount)) : 100000ull;

    World world;
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<Entity> entities;
    entities.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        entities.push_back(world.CreateEntity());
    }
    const auto tCreate = std::chrono::steady_clock::now();

    std::size_t iterated = 0;
    world.QueryAll<TransformComponent>().Each([&](Entity, TransformComponent& t) {
        t.localPosition.x += 1.0f;
        t.dirty = true;
        ++iterated;
    });
    const auto tQuery = std::chrono::steady_clock::now();

    // Structural churn on a subset (POD components — safe across migrations)
    for (std::size_t i = 0; i < 10000 && i < entities.size(); ++i) {
        world.AddComponent<DirectionalLightComponent>(entities[i], DirectionalLightComponent{});
    }
    for (std::size_t i = 0; i < 5000 && i < entities.size(); ++i) {
        world.RemoveComponent<DirectionalLightComponent>(entities[i]);
    }
    const auto tMigrate = std::chrono::steady_clock::now();

    // Parallel chunk pass
    world.QueryAll<TransformComponent>().ParallelEachChunk(world.Jobs(), [](ChunkView chunk) {
        TransformComponent* transforms = chunk.Column<TransformComponent>();
        if (!transforms) {
            return;
        }
        for (std::uint16_t s = 0; s < chunk.Count(); ++s) {
            transforms[s].localScale.x = 1.0f;
        }
    });
    const auto tParallel = std::chrono::steady_clock::now();

    const auto ms = [](auto a, auto b) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
    };

    std::cout << "Stress count=" << count
              << " create=" << ms(t0, tCreate) << "ms"
              << " query=" << ms(tCreate, tQuery) << "ms"
              << " migrate=" << ms(tQuery, tMigrate) << "ms"
              << " parallel=" << ms(tMigrate, tParallel) << "ms"
              << " living=" << world.Entities().LivingCount()
              << " chunks=" << world.Chunks().AllocatedChunkCount()
              << "\n";

    CHECK(iterated == count, "iterated all entities");
    CHECK(world.Entities().LivingCount() == count, "living count matches");
    CHECK(world.Chunks().AllocatedChunkCount() > 0, "chunks allocated");
}

} // namespace

int main() {
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    std::cout << "ECS production validation\n";
    TestCreateDestroy();
    TestArchetypeMigration();
    TestCommandBuffer();
    TestHierarchyTransform();
    TestSharedStore();
    TestRenderExtract();
    TestSceneEcsAuthority();
    TestSerializationHooks();
    TestMillionEntities();

    std::cout << "Passed=" << g_Passed << " Failed=" << g_Failed << "\n";
    return g_Failed == 0 ? 0 : 1;
}
