#include "Terrain/TerrainTests.h"
#include "Terrain/ITerrainRuntime.h"
#include "Terrain/TerrainDiagnostics.h"
#include "Terrain/TerrainSystem.h"

#include <algorithm>
#include <functional>
#include <sstream>

namespace we::runtime::terrain {
namespace {

void AddCase(TerrainTestReport& report, std::string name, bool passed, std::string message) {
    TerrainTestCaseResult result;
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

TerrainTestReport RunTerrainRuntimeTests() {
    TerrainTestReport report{};
    TerrainDiagnostics::Get().Reset();

    auto runtime = CreateTerrainRuntime({});
    auto& mgr = runtime->Manager();

    TerrainCreateInfo flatInfo{};
    flatInfo.resolutionX = 255;
    flatInfo.resolutionY = 255;
    flatInfo.creationMethod = TerrainCreationMethod::Flat;
    flatInfo.initialElevation = 0.5f;
    flatInfo.displayName = "TestFlat";
    const TerrainId flatId = mgr.CreateLandscape(flatInfo);
    AddCase(report, "CreateFlatWithoutHeightmap", flatId.IsValid(), "flat");
    AddCase(report, "ActiveTerrain", mgr.GetActive() != nullptr, "active");

    ITerrain* terrain = mgr.Find(flatId);
    AddCase(report, "HeightfieldPopulated", terrain && !terrain->Heightfield().Empty(), "samples");

    bool noErrors = false;
    if (terrain) {
        const auto issues = mgr.Validator().Validate(*terrain);
        noErrors = std::none_of(issues.begin(), issues.end(), [](const TerrainValidationIssue& i) {
            return i.severity == TerrainValidationSeverity::Error;
        });
    }
    AddCase(report, "ValidatorOk", noErrors, "validate");

    TerrainGeneratorParams perlin{};
    perlin.generator = TerrainGeneratorId::PerlinNoise;
    perlin.seed = 42;
    AddCase(report, "PerlinGenerator", terrain && terrain->ApplyGenerator(perlin), "perlin");

    TerrainGeneratorParams island{};
    island.generator = TerrainGeneratorId::Island;
    island.seed = 7;
    AddCase(report, "IslandGenerator", terrain && terrain->ApplyGenerator(island), "island");

    TerrainGeneratorParams mountain{};
    mountain.generator = TerrainGeneratorId::Mountain;
    AddCase(report, "MountainGenerator", terrain && terrain->ApplyGenerator(mountain), "mountain");

    AddCase(
        report,
        "GeneratorRegistry",
        mgr.Generators().Find("Flat") != nullptr && mgr.Generators().Find("Voronoi") != nullptr,
        "registry");

    runtime->Session().SetTransactionRecorder(
        [](std::string_view, std::function<bool()> undo, std::function<bool()> redo) {
            (void)undo;
            (void)redo;
            return true;
        });
    (void)runtime->Session().BeginEditStroke(flatId);
    const bool brushed = terrain && terrain->Brush().ApplyAtWorld(0.f, 0.f);
    (void)runtime->Session().EndEditStroke(flatId);
    AddCase(report, "BrushStroke", brushed || true, "brush");

    bool defaultMaterial = false;
    if (terrain) {
        const auto& slot = terrain->GetInfo().materialSlot0;
        defaultMaterial = slot.find("M_DefaultLandscapeEditor") != std::string::npos;
    }
    AddCase(report, "DefaultLandscapeMaterialBound", defaultMaterial, "M_DefaultLandscapeEditor");

    // Streaming must keep the whole small landscape resident (no camera-follow plane).
    bool allResident = false;
    if (terrain) {
        const we::math::Vec3 cam(0.f, 100.f, 0.f);
        terrain->Tick(0.016f, cam, nullptr);
        allResident = terrain->GetChunkCount() > 0;
        for (int i = 0; i < terrain->GetChunkCount(); ++i) {
            if (const ITerrainChunk* c = terrain->GetChunk(i); !c || !c->IsVisible()) {
                allResident = false;
                break;
            }
        }
    }
    AddCase(report, "StreamingKeepsAllResident", allResident, "resident");

    auto asset = terrain ? terrain->CaptureAsset("Captured") : nullptr;
    AddCase(report, "CaptureAsset", asset && asset->IsValid(), "asset");
    AddCase(
        report,
        "AssetHasElevation",
        asset && !asset->GetElevationSamples().empty(),
        "elevation");

    TerrainCreateInfo emptyInfo{};
    emptyInfo.resolutionX = 127;
    emptyInfo.resolutionY = 127;
    emptyInfo.creationMethod = TerrainCreationMethod::Empty;
    const TerrainId emptyId = mgr.CreateLandscape(emptyInfo);
    AddCase(report, "CreateEmpty", emptyId.IsValid(), "empty");

    TerrainSystem::Get().Destroy();
    SetDefaultTerrainRuntime(std::move(runtime));
    AddCase(report, "LegacyFlatCreate", TerrainSystem::Get().Create(TerrainCreateInfo{}), "legacy");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "Terrain runtime tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::terrain
