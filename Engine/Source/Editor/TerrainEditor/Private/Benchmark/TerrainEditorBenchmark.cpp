#include "TerrainEditor/TerrainEditorBenchmark.h"
#include "Terrain/TerrainSystem.h"
#include "Terrain/TerrainProcedural.h"

#include <chrono>
#include <sstream>

namespace we::editor::terrain {

TerrainEditorBenchmarkReport RunTerrainEditorBenchmarks(
    std::uint32_t resolution,
    std::uint32_t brushIterations)
{
    TerrainEditorBenchmarkReport report{};
    report.resolution = resolution;
    report.brushIterations = brushIterations;

    using clock = std::chrono::steady_clock;
    auto& terrain = runtime_terrain::TerrainSystem::Get();
    terrain.Destroy();

    runtime_terrain::TerrainCreateInfo info{};
    info.resolutionX = static_cast<int>(resolution);
    info.resolutionY = static_cast<int>(resolution);
    info.worldSizeX = static_cast<float>(resolution);
    info.worldSizeY = static_cast<float>(resolution);

    auto t0 = clock::now();
    (void)terrain.Create(info);
    report.createMs =
        std::chrono::duration<double, std::milli>(clock::now() - t0).count();

    runtime_terrain::ProceduralHeightmapParams proc{};
    t0 = clock::now();
    (void)terrain.GenerateProcedural(proc);
    report.proceduralMs =
        std::chrono::duration<double, std::milli>(clock::now() - t0).count();

    terrain.Brushes().SetSettings(runtime_terrain::TerrainBrushSettings{});
    t0 = clock::now();
    for (std::uint32_t i = 0; i < brushIterations; ++i) {
        const float x = static_cast<float>(i % 32) * 4.f;
        const float z = static_cast<float>(i / 32) * 4.f;
        (void)terrain.ApplyBrushWorld(x, z);
    }
    report.brushMs =
        std::chrono::duration<double, std::milli>(clock::now() - t0).count();

    std::ostringstream oss;
    oss << "TerrainEditor bench res=" << resolution
        << " createMs=" << report.createMs
        << " proceduralMs=" << report.proceduralMs
        << " brushMs=" << report.brushMs
        << " iters=" << brushIterations;
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::terrain
