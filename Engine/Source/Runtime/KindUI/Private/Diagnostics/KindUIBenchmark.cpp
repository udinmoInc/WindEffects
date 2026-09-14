// ==============================================================================
// WindEffects — KindUI — KindUIBenchmark
// Minimal headless layout/paint gate benchmark (env: WE_UI_BENCH).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIBenchmark.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"

#include <chrono>
#include <sstream>

namespace we::runtime::kindui {

KindUIBenchmarkReport RunKindUIBenchmark(uint32_t iterations) {
    KindUIBenchmarkReport report{};
    report.iterations = iterations;

    auto root = std::make_shared<Flex>(FlexDirection::Column);
    for (int i = 0; i < 32; ++i) {
        root->AddChild(std::make_shared<Label>("Row " + std::to_string(i)));
    }
    report.widgetCount = 33;

    const Rect viewport{ 0.0f, 0.0f, 640.0f, 480.0f };
    using clock = std::chrono::steady_clock;
    double layoutAccum = 0.0;
    double paintAccum = 0.0;
    double idleAccum = 0.0;
    PaintContext paint;

    for (uint32_t i = 0; i < iterations; ++i) {
        const auto t0 = clock::now();
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        const auto t1 = clock::now();
        paint.Clear();
        root->Paint(paint);
        const auto t2 = clock::now();
        paint.Clear();
        root->Paint(paint);
        const auto t3 = clock::now();
        layoutAccum += static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
        paintAccum += static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
        idleAccum += static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count());
    }

    const double n = static_cast<double>(std::max(1u, iterations));
    report.layoutMicros = layoutAccum / n;
    report.paintMicros = paintAccum / n;
    report.idleFrameMicros = idleAccum / n;
    report.hoverFrameMicros = report.idleFrameMicros;
    report.fullRebuildMicros = report.layoutMicros + report.paintMicros;

    std::ostringstream oss;
    oss << "KindUI bench iters=" << iterations
        << " layout=" << static_cast<uint64_t>(report.layoutMicros) << "us"
        << " paint=" << static_cast<uint64_t>(report.paintMicros) << "us"
        << " idle=" << static_cast<uint64_t>(report.idleFrameMicros) << "us";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::kindui
