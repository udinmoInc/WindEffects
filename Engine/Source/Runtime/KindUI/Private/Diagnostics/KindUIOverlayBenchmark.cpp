// ==============================================================================
// WindEffects — KindUI — KindUIOverlayBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIOverlayBenchmark.h"

#include "KindUI/Core/LayoutIncremental.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"
#include "KindUI/UI/OverlayManager.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace we::runtime::kindui {
namespace {

using clock = std::chrono::steady_clock;

double Micros(clock::time_point a, clock::time_point b) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
}

struct Accumulators {
    double updateUs = 0.0;
    double frameUs = 0.0;
    uint64_t overlayOnly = 0;
    uint64_t fullHost = 0;
    uint64_t slotsMeasured = 0;
    uint64_t slotsArranged = 0;
    uint64_t baseSkipped = 0;
    uint64_t repositionSkipped = 0;
    uint64_t measureRan = 0;
    uint64_t arrangeRan = 0;
    double dirtyCoverage = 0.0;
    uint64_t dirtyRegions = 0;
};

std::shared_ptr<Column> MakeBaseTree(int rows) {
    auto root = std::make_shared<Column>();
    root->Gap(2.0f);
    for (int i = 0; i < rows; ++i) {
        root->AddChild(std::make_shared<Label>("Panel row " + std::to_string(i)));
    }
    return root;
}

std::shared_ptr<Column> MakeMenu(int items) {
    auto menu = std::make_shared<Column>();
    menu->Gap(1.0f);
    for (int i = 0; i < items; ++i) {
        menu->AddChild(std::make_shared<Label>("Menu item " + std::to_string(i)));
    }
    return menu;
}

void SnapshotOverlayStats(Accumulators& acc) {
    const auto& s = OverlayHost::Stats();
    acc.overlayOnly += s.overlayOnlyLayouts;
    acc.fullHost += s.fullHostArranges;
    acc.slotsMeasured += s.slotsMeasured;
    acc.slotsArranged += s.slotsArranged;
    acc.baseSkipped += s.baseArrangesSkipped;
    acc.repositionSkipped += s.repositionSkipped;
}

void ClearLayoutGates() {
    (void)UIRepaintGate::ConsumeNeedsLayout();
    (void)UIRepaintGate::ConsumeNeedsOverlayLayout();
    (void)UIRepaintGate::ConsumeNeedsPaint();
    UIDirtyRegionTracker::Get().Reset();
    OverlayHost::ResetStats();
}

void RunOverlayFrame(
    const std::shared_ptr<OverlayHost>& host,
    const Rect& viewport,
    Accumulators& acc)
{
    OverlayHost::ResetStats();
    LayoutIncrementalStats::ResetCurrent();

    const auto t0 = clock::now();
    if (UIRepaintGate::ConsumeNeedsLayout()) {
        ++LayoutIncrementalStats::Current().fullLayoutPasses;
        host->Measure(Size{ viewport.width, viewport.height });
        host->Arrange(viewport);
        host->ClearSubtreeLayoutDirty();
        UIRepaintGate::NotifyHostLayoutCompleted();
    } else if (UIRepaintGate::ConsumeNeedsOverlayLayout()) {
        host->SyncOverlaysOnly();
    }
    const auto t1 = clock::now();
    acc.updateUs += Micros(t0, t1);

    PaintContext paint;
    paint.Clear();
    paint.SetPaintRetentionEnabled(true);
    host->PaintSubtree(paint);
    const auto t2 = clock::now();
    acc.frameUs += Micros(t0, t2);

    const DirtyRegionFrameStats dirty = UIDirtyRegionTracker::Get().ConsumeMerged(viewport);
    if (dirty.rawAddCount > 0) {
        acc.dirtyCoverage += dirty.coverage;
        acc.dirtyRegions += dirty.regionCount;
    }

    const auto& lay = LayoutIncrementalStats::Current();
    acc.measureRan += lay.measureRan;
    acc.arrangeRan += lay.arrangeRan;
    SnapshotOverlayStats(acc);

    (void)UIRepaintGate::ConsumeNeedsPaint();
    (void)UIRepaintGate::ConsumeNeedsLayout();
    (void)UIRepaintGate::ConsumeNeedsOverlayLayout();
}

OverlayScenarioResult Finish(const char* name, uint32_t iterations, const Accumulators& acc) {
    OverlayScenarioResult result{};
    result.name = name;
    result.iterations = iterations;
    const double n = static_cast<double>((std::max)(1u, iterations));
    result.avgUpdateMicros = acc.updateUs / n;
    result.avgFrameMicros = acc.frameUs / n;
    result.avgOverlayOnlyLayouts = static_cast<uint32_t>(acc.overlayOnly / static_cast<uint64_t>(iterations));
    result.avgFullHostArranges = static_cast<uint32_t>(acc.fullHost / static_cast<uint64_t>(iterations));
    result.avgSlotsMeasured = static_cast<uint32_t>(acc.slotsMeasured / static_cast<uint64_t>(iterations));
    result.avgSlotsArranged = static_cast<uint32_t>(acc.slotsArranged / static_cast<uint64_t>(iterations));
    result.avgBaseArrangesSkipped = static_cast<uint32_t>(acc.baseSkipped / static_cast<uint64_t>(iterations));
    result.avgRepositionSkipped = static_cast<uint32_t>(acc.repositionSkipped / static_cast<uint64_t>(iterations));
    result.avgMeasureRan = static_cast<uint32_t>(acc.measureRan / static_cast<uint64_t>(iterations));
    result.avgArrangeRan = static_cast<uint32_t>(acc.arrangeRan / static_cast<uint64_t>(iterations));
    result.avgDirtyCoverage = static_cast<float>(acc.dirtyCoverage / n);
    result.avgDirtyRegions = static_cast<uint32_t>(acc.dirtyRegions / static_cast<uint64_t>(iterations));
    return result;
}

std::shared_ptr<OverlayHost> MakeHost(int baseRows, const Rect& viewport) {
    auto host = std::make_shared<OverlayHost>();
    host->SetBaseWidget(MakeBaseTree(baseRows));
    host->Measure(Size{ viewport.width, viewport.height });
    host->Arrange(viewport);
    host->ClearSubtreeLayoutDirty();
    ClearLayoutGates();
    return host;
}

} // namespace

KindUIOverlayReport RunKindUIOverlayBenchmark(uint32_t iterations) {
    KindUIOverlayReport report{};
    const Rect viewport{ 0.0f, 0.0f, 1280.0f, 720.0f };
    const uint32_t n = (std::max)(1u, iterations);

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            UIRepaintGate::RequestOverlayLayoutReason("BenchIdle");
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("idle_overlay_sync", n, acc));
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseAllPopups();
            auto menu = MakeMenu(10);
            ClearLayoutGates();
            host->ShowAnchoredPopup(menu, Rect{ 40.0f, 40.0f, 120.0f, 24.0f });
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("open_dropdown", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseAllPopups();
            auto menu = MakeMenu(8);
            ClearLayoutGates();
            host->ShowPopup(menu, Point{ 200.0f, 160.0f });
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("open_context", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseTooltips();
            auto tip = std::make_shared<Label>("Tooltip " + std::to_string(i));
            ClearLayoutGates();
            host->ShowTooltip(tip, Rect{ 80.0f, 90.0f, 24.0f, 24.0f });
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("open_tooltip", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        auto anchor = std::make_shared<Label>("Anchor");
        host->GetBaseWidget()->AddChild(anchor);
        host->Measure(Size{ viewport.width, viewport.height });
        host->Arrange(viewport);
        host->ClearSubtreeLayoutDirty();
        auto menu = MakeMenu(8);
        ClearLayoutGates();
        host->ShowAnchoredPopup(menu, anchor);
        Accumulators warm{};
        RunOverlayFrame(host, viewport, warm);

        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            anchor->Arrange(Rect{ 32.0f, 40.0f + static_cast<float>(i % 30) * 6.0f, 100.0f, 22.0f });
            UIDirtyRegionTracker::Get().Reset();
            UIRepaintGate::RequestOverlayLayoutReason("BenchScroll");
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("scroll_reposition", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseAllPopups();
            auto menu = MakeMenu(8);
            auto tip = std::make_shared<Label>("Stacked tip");
            ClearLayoutGates();
            host->ShowAnchoredPopup(menu, Rect{ 60.0f, 50.0f, 140.0f, 24.0f });
            host->ShowTooltip(tip, Rect{ 220.0f, 80.0f, 20.0f, 20.0f });
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("stacked_overlays", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            auto menu = MakeMenu(6);
            ClearLayoutGates();
            host->ShowPopup(menu, Point{ 100.0f, 100.0f });
            host->CloseTopPopup();
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("open_close_cycle", n, acc));
    }

    {
        auto host = MakeHost(280, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseAllPopups();
            auto menu = MakeMenu(8);
            ClearLayoutGates();
            host->ShowAnchoredPopup(menu, Rect{ 50.0f, 50.0f, 100.0f, 24.0f });
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("large_base_isolation", n, acc));
        host->CloseAllPopups();
    }

    {
        auto host = MakeHost(120, viewport);
        Accumulators acc{};
        for (uint32_t i = 0; i < n; ++i) {
            host->CloseAllPopups();
            auto modal = std::make_shared<Column>();
            modal->AddChild(std::make_shared<Label>("Modal " + std::to_string(i)));
            ClearLayoutGates();
            host->ShowModal(modal);
            RunOverlayFrame(host, viewport, acc);
        }
        report.scenarios.push_back(Finish("open_modal", n, acc));
        host->CloseAllPopups();
    }

    std::ostringstream summary;
    summary << "overlay scenarios=" << report.scenarios.size();
    for (const auto& s : report.scenarios) {
        summary << " | " << s.name
            << " upd=" << static_cast<uint64_t>(s.avgUpdateMicros) << "us"
            << " ovl=" << s.avgOverlayOnlyLayouts
            << " full=" << s.avgFullHostArranges
            << " baseSkip=" << s.avgBaseArrangesSkipped
            << " dirty=" << static_cast<uint64_t>(s.avgDirtyCoverage * 100.0f) << "%";
    }
    report.summary = summary.str();
    return report;
}

} // namespace we::runtime::kindui
