#include "Terrain/TerrainBenchmark.h"
#include "Terrain/ITerrainRuntime.h"

#include <chrono>
#include <functional>
#include <sstream>

namespace we::runtime::terrain {
namespace {

TerrainBenchmarkResult Bench(
    std::string name,
    std::uint64_t iterations,
    const std::function<void()>& fn)
{
    const auto t0 = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < iterations; ++i) {
        fn();
    }
    const auto micros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0)
            .count());
    TerrainBenchmarkResult r;
    r.name = std::move(name);
    r.iterations = iterations;
    r.totalMicros = micros;
    r.microsPerOp = iterations > 0 ? static_cast<double>(micros) / static_cast<double>(iterations) : 0.0;
    return r;
}

} // namespace

TerrainBenchmarkReport RunTerrainRuntimeBenchmarks() {
    TerrainBenchmarkReport report{};
    auto runtime = CreateTerrainRuntime({});
    auto& mgr = runtime->Manager();

    TerrainCreateInfo info{};
    info.resolutionX = 505;
    info.resolutionY = 505;
    info.creationMethod = TerrainCreationMethod::Flat;

    report.results.push_back(Bench("CreateFlat505", 4, [&]() {
        const TerrainId id = mgr.CreateLandscape(info);
        (void)mgr.DestroyLandscape(id);
    }));

    const TerrainId id = mgr.CreateLandscape(info);
    mgr.SetActive(id);
    ITerrain* terrain = mgr.Find(id);

    TerrainGeneratorParams fbm{};
    fbm.generator = TerrainGeneratorId::Fbm;
    fbm.seed = 99;
    report.results.push_back(Bench("GenerateFbm505", 3, [&]() {
        if (terrain) {
            (void)terrain->ApplyGenerator(fbm);
        }
    }));

    TerrainGeneratorParams voronoi{};
    voronoi.generator = TerrainGeneratorId::Voronoi;
    report.results.push_back(Bench("GenerateVoronoi505", 3, [&]() {
        if (terrain) {
            (void)terrain->ApplyGenerator(voronoi);
        }
    }));

    if (terrain) {
        terrain->Brush().SetSettings(TerrainBrushSettings{});
    }
    report.results.push_back(Bench("BrushApply", 64, [&]() {
        if (terrain) {
            (void)terrain->Brush().ApplyAtWorld(10.f, 10.f);
        }
    }));

    std::ostringstream oss;
    oss << "Terrain benchmarks (" << report.results.size() << " cases)";
    for (const auto& r : report.results) {
        oss << "\n  " << r.name << ": " << r.microsPerOp << " us/op";
    }
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::terrain
