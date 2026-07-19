#include "WorldOutliner/WorldOutlinerBenchmark.h"
#include "WorldOutliner/WorldOutliner.h"
#include "Scene/Scene.h"

#include <chrono>
#include <sstream>

namespace we::editor::outliner {

WorldOutlinerBenchmarkReport RunWorldOutlinerBenchmarks(std::uint32_t entityCount, std::uint32_t iterations) {
    WorldOutlinerBenchmarkReport report{};
    report.entityCount = entityCount;

    auto scene = std::make_shared<scene::Scene>();
    scene->CreateEntity("Root", scene::EntityType::EmptyActor);
    const auto rootId = scene->GetEntities().front().Id;
    for (std::uint32_t i = 0; i < entityCount; ++i) {
        scene->CreateEntity("Actor" + std::to_string(i), scene::EntityType::Cube);
        scene->GetEntities().back().ParentId = rootId;
    }

    WorldOutlinerDependencies deps;
    deps.scene = scene.get();
    auto runtime = CreateWorldOutlinerRuntime(deps);
    auto& outliner = runtime->Outliner();

    using clock = std::chrono::steady_clock;
    {
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            outliner.RequestRebuild();
            outliner.Tick(0.f);
        }
        const auto t1 = clock::now();
        report.rebuildIterations = iterations;
        report.rebuildTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        OutlinerFilterState filter;
        filter.searchQuery = "Actor1";
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            filter.searchQuery = "Actor" + std::to_string(i % 100);
            outliner.SetFilterState(filter);
            outliner.Tick(0.f);
        }
        const auto t1 = clock::now();
        report.filterIterations = iterations;
        report.filterTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    std::ostringstream oss;
    oss << "WorldOutliner bench entities=" << entityCount
        << " rebuild=" << report.rebuildTotalMicros << "us/" << report.rebuildIterations
        << " filter=" << report.filterTotalMicros << "us/" << report.filterIterations;
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::outliner
