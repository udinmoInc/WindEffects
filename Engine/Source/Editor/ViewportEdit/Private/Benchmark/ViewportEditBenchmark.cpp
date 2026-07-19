#include "ViewportEdit/ViewportEditBenchmark.h"
#include "ViewportEdit/ViewportEdit.h"
#include "Scene/Scene.h"
#include "EditorCamera.h"

#include <chrono>
#include <sstream>

namespace we::editor::viewportedit {

ViewportEditBenchmarkReport RunViewportEditBenchmarks(std::uint32_t entityCount, std::uint32_t iterations) {
    ViewportEditBenchmarkReport report{};
    auto scene = std::make_shared<scene::Scene>();
    auto camera = std::make_shared<engine::EditorCamera>();
    camera->SetViewportSize(1280.f, 720.f);

    for (std::uint32_t i = 0; i < entityCount; ++i) {
        scene->CreateEntity("E" + std::to_string(i), scene::EntityType::Cube);
        auto& e = scene->GetEntities().back();
        e.Position = {
            static_cast<float>(i % 32) * 2.f,
            0.f,
            static_cast<float>(i / 32) * 2.f};
    }

    ViewportEditDependencies deps;
    deps.scene = scene.get();
    deps.editorCamera = camera.get();
    auto editor = CreateViewportEditRuntime(deps);
    editor->SetViewportSize(1280.f, 720.f);

    using clock = std::chrono::steady_clock;
    {
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            (void)editor->HitTester().Pick(
                static_cast<float>(i % 1280),
                static_cast<float>((i * 7) % 720),
                1280.f,
                720.f);
        }
        const auto t1 = clock::now();
        report.pickIterations = iterations;
        report.pickTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        if (!scene->GetEntities().empty()) {
            editor->Selection().Set(ViewportObjectId{scene->GetEntities().front().Id});
        }
        editor->SetActiveTool(ViewportToolId::Move);
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            editor->Manipulator().ApplyTranslation({0.01f, 0.f, 0.f});
        }
        const auto t1 = clock::now();
        report.transformIterations = iterations;
        report.transformTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    std::ostringstream oss;
    oss << "ViewportEdit bench entities=" << entityCount
        << " pick=" << report.pickTotalMicros << "us/" << report.pickIterations
        << " xform=" << report.transformTotalMicros << "us/" << report.transformIterations;
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::viewportedit
