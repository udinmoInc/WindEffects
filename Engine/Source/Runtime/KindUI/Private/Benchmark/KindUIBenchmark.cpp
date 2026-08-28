#include "KindUI/Benchmark/KindUIBenchmark.h"

#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Core/Widgets/PanelToolbarRow.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Widgets/VirtualList.h"

#include <chrono>
#include <functional>
#include <sstream>

namespace we::runtime::kindui {

namespace {

std::shared_ptr<Column> BuildEditorLikeTree() {
    auto root = std::make_shared<Column>();
    root->Gap(ResolveMetric(MetricToken::Space2));

    auto toolbar = std::make_shared<PanelToolbarRow>("Search Assets...");
    toolbar->AddIconButton(Icons::FilterName, []() {});
    toolbar->AddIconButton(Icons::PlusName, []() {});
    toolbar->AddIconButton(Icons::RefreshName, []() {});
    toolbar->Finalize();
    root->AddChild(toolbar);

    auto listHost = std::make_shared<Column>();
    listHost->Gap(ResolveMetric(MetricToken::Space1));
    for (int i = 0; i < 64; ++i) {
        auto row = std::make_shared<PropertyRow>("Property " + std::to_string(i), "Value");
        listHost->AddChild(row);
    }
    root->AddChild(listHost);

    return root;
}

double MeasureMicros(const std::function<void()>& fn, uint32_t iterations) {
    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();
    for (uint32_t i = 0; i < iterations; ++i) {
        fn();
    }
    const auto t1 = clock::now();
    return std::chrono::duration<double, std::micro>(t1 - t0).count();
}

} // namespace

KindUIBenchmarkReport RunKindUIBenchmark(const uint32_t iterations) {
    KindUIBenchmarkReport report{};
    report.iterations = iterations;

    ThemeManager::Get().Initialize(std::make_shared<DefaultTheme>(), 1.0f);

    auto root = BuildEditorLikeTree();
    report.widgetCount = 70;

    constexpr float width = 1280.0f;
    constexpr float height = 720.0f;
    const Rect viewport{0.0f, 0.0f, width, height};

    report.layoutMicros = MeasureMicros([&]() {
        UIRepaintGate::RequestLayout();
        (void)UIRepaintGate::ConsumeNeedsLayout();
        root->Measure(Size{width, height});
        root->Arrange(viewport);
        root->ClearLayoutDirty();
    }, iterations);

    report.paintMicros = MeasureMicros([&]() {
        UIRepaintGate::RequestPaint();
        (void)UIRepaintGate::ConsumeNeedsPaint();
        PaintContext ctx;
        root->Paint(ctx);
        (void)ctx.GetCommands().size();
    }, iterations);

    report.fullRebuildMicros = MeasureMicros([&]() {
        UIRepaintGate::Request();
        (void)UIRepaintGate::ConsumeNeedsLayout();
        (void)UIRepaintGate::ConsumeNeedsPaint();
        root->Measure(Size{width, height});
        root->Arrange(viewport);
        root->ClearLayoutDirty();
        PaintContext ctx;
        root->Paint(ctx);
        (void)ctx.GetCommands().size();
    }, iterations);

    const uint64_t skipStart = UIRepaintGate::IdleSkipCount();
    const uint64_t paintStart = UIRepaintGate::PaintRebuildCount();
    const uint64_t layoutStart = UIRepaintGate::LayoutRebuildCount();

    report.idleFrameMicros = MeasureMicros([&]() {
        (void)UIRepaintGate::ConsumeNeedsLayout();
        (void)UIRepaintGate::ConsumeNeedsPaint();
    }, iterations * 4);

    report.hoverFrameMicros = MeasureMicros([&]() {
        UIRepaintGate::RequestPaint();
        (void)UIRepaintGate::ConsumeNeedsLayout();
        if (UIRepaintGate::ConsumeNeedsPaint()) {
            PaintContext ctx;
            root->Paint(ctx);
            (void)ctx.GetCommands().size();
        }
    }, iterations);

    report.idleSkips = UIRepaintGate::IdleSkipCount() - skipStart;
    report.paintRebuilds = UIRepaintGate::PaintRebuildCount() - paintStart;
    report.layoutRebuilds = UIRepaintGate::LayoutRebuildCount() - layoutStart;

    std::ostringstream oss;
    oss << "KindUI bench iters=" << iterations
        << " layout=" << static_cast<uint64_t>(report.layoutMicros / iterations) << "us"
        << " paint=" << static_cast<uint64_t>(report.paintMicros / iterations) << "us"
        << " idle=" << static_cast<uint64_t>(report.idleFrameMicros / (iterations * 4)) << "us"
        << " hover=" << static_cast<uint64_t>(report.hoverFrameMicros / iterations) << "us"
        << " full=" << static_cast<uint64_t>(report.fullRebuildMicros / iterations) << "us";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::kindui
