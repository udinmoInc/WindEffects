#include "Prefab/PrefabBenchmark.h"
#include "Prefab/Prefab.h"
#include "Scene/Scene.h"

#include <chrono>
#include <filesystem>
#include <sstream>

namespace we::runtime::prefab {

PrefabBenchmarkReport RunPrefabBenchmarks(
    std::uint32_t spawnIterations,
    std::uint32_t serializeIterations)
{
    PrefabBenchmarkReport report{};
    report.spawnIterations = spawnIterations;
    report.serializeIterations = serializeIterations;

    scene::Scene scene;
    scene.CreateEntity("BenchRoot", scene::EntityType::Cube);
    if (scene.GetEntities().empty()) {
        report.summary = "Prefab bench failed: empty scene";
        return report;
    }

    PrefabDependencies deps;
    deps.scene = &scene;
    auto runtime = CreatePrefabRuntime(deps);
    auto& mgr = runtime->Manager();

    PrefabCommandContext createCtx;
    createCtx.rootEntityId = scene.GetEntities().front().Id;
    createCtx.name = "BenchPrefab";
    createCtx.path = (std::filesystem::temp_directory_path() / "we_prefab_bench.weprefab").string();
    (void)mgr.Commands().Execute(PrefabCommandId::CreateFromSelection, createCtx);
    const auto guids = mgr.Registry().ListAll();
    if (guids.empty()) {
        report.summary = "Prefab bench failed: no asset";
        return report;
    }
    const PrefabGuid guid = guids.front();

    using clock = std::chrono::steady_clock;
    {
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < spawnIterations; ++i) {
            PrefabSpawnParams spawn;
            spawn.prefab = guid;
            spawn.instanceName = "I" + std::to_string(i);
            const auto id = mgr.Spawner().Spawn(spawn);
            if (id.IsValid()) {
                (void)mgr.Spawner().Despawn(id);
            }
        }
        const auto t1 = clock::now();
        report.spawnTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        auto asset = mgr.Registry().Find(guid);
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < serializeIterations; ++i) {
            auto bytes = mgr.Serializer().SerializeBytes(*asset);
            (void)mgr.Serializer().DeserializeBytes(bytes);
        }
        const auto t1 = clock::now();
        report.serializeTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    runtime->Shutdown();
    std::error_code ec;
    std::filesystem::remove(createCtx.path, ec);

    std::ostringstream oss;
    oss << "Prefab bench spawn=" << report.spawnTotalMicros << "us/" << spawnIterations
        << " serialize=" << report.serializeTotalMicros << "us/" << serializeIterations;
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::prefab
