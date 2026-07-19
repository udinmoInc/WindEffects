#include "World/WorldBenchmark.h"
#include "World/World.h"

#include <chrono>
#include <cstring>
#include <functional>
#include <sstream>

namespace we::runtime::world {
namespace {

WorldBenchmarkSample Measure(std::string name, std::uint64_t iterations, const std::function<void()>& fn) {
    const auto t0 = std::chrono::steady_clock::now();
    fn();
    const auto t1 = std::chrono::steady_clock::now();
    WorldBenchmarkSample sample;
    sample.name = std::move(name);
    sample.iterations = iterations;
    sample.milliseconds = std::chrono::duration<double, std::milli>(t1 - t0).count();
    sample.perIterationNs = iterations == 0
        ? 0.0
        : (sample.milliseconds * 1.0e6) / static_cast<double>(iterations);
    return sample;
}

} // namespace

WorldBenchmarkReport RunWorldRuntimeBenchmarks(const WorldBenchmarkConfig& config) {
    WorldBenchmarkReport report{};
    WorldDiagnostics::Get().Reset();

    auto runtime = CreateWorldRuntime({});
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create runtime";
        return report;
    }

    WorldCreateInfo info{};
    std::memcpy(info.descriptor.name, "BenchWorld", 11);
    const WorldHandle handle = runtime->Manager().CreateWorld(info);
    IWorld* world = runtime->Manager().FindWorld(handle);
    if (!world) {
        report.success = false;
        report.summary = "Failed to create world";
        return report;
    }

    const std::uint32_t count = config.runStress ? config.actorCount * 10u : config.actorCount;

    report.samples.push_back(Measure("SpawnActors", count, [&]() {
        for (std::uint32_t i = 0; i < count; ++i) {
            ActorSpawnParams spawn{};
            spawn.localPosition[0] = static_cast<float>(i % 100);
            spawn.localPosition[1] = static_cast<float>((i / 100) % 100);
            spawn.localPosition[2] = static_cast<float>(i / 10000);
            spawn.beginPlayImmediately = false;
            (void)world->Actors().Spawn(spawn);
        }
    }));

    report.samples.push_back(Measure("Tick60", 60, [&]() {
        WorldTickParams tick{};
        tick.deltaSeconds = 1.f / 60.f;
        for (int i = 0; i < 60; ++i) {
            world->Tick(tick);
        }
    }));

    report.samples.push_back(Measure("HierarchyOps", count, [&]() {
        const auto all = world->Actors().All();
        if (all.size() < 2) {
            return;
        }
        const ActorHandle root = all[0];
        for (std::size_t i = 1; i < all.size(); ++i) {
            (void)world->Hierarchy().SetParent(all[i], root);
        }
    }));

    report.samples.push_back(Measure("QueryAll", config.queryIterations, [&]() {
        ActorQueryFilter filter{};
        for (std::uint32_t i = 0; i < config.queryIterations; ++i) {
            (void)world->Queries().Query(filter);
        }
    }));

    report.samples.push_back(Measure("SpatialRebuild+Query", config.queryIterations, [&]() {
        world->Spatial().Rebuild();
        Sphere3f sphere{};
        sphere.radius = 50.f;
        for (std::uint32_t i = 0; i < config.queryIterations; ++i) {
            (void)world->Spatial().OverlapSphere(sphere);
        }
    }));

    report.samples.push_back(Measure("SaveLoad", 1, [&]() {
        auto bytes = world->Persistence().SaveWorld({});
        (void)world->Persistence().LoadWorld(bytes);
    }));

    runtime->Shutdown();
    report.diagnostics = WorldDiagnostics::Get().Capture();
    report.success = true;

    std::ostringstream oss;
    oss << "WorldRuntimeBenchmarks: " << report.samples.size() << " samples, actors="
        << report.diagnostics.actorCount;
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::world
