// ==============================================================================
// WindEffects — KindUI — KindUIDirtyRegionBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIDirtyRegionBenchmark.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIDirtyRegionTracker.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"
#include "KindUI/UI/VirtualList.h"
#include "KindUI/Theme/ThemeAccess.h"

#include <chrono>
#include <functional>
#include <sstream>

namespace we::runtime::kindui {

namespace {

using clock = std::chrono::steady_clock;

double Micros(clock::time_point a, clock::time_point b) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
}

class BenchRow final : public Widget {
public:
    explicit BenchRow(size_t index = 0) : m_Index(index) {}
    void Rebind(size_t index) {
        m_Index = index;
        InvalidatePaint();
    }
    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ availableSize.width, 24.0f };
        return m_DesiredSize;
    }
    void Arrange(const Rect& allottedRect) override {
        CommitGeometry(allottedRect);
        ClearLayoutDirty();
    }
    void Paint(PaintContext& context) override {
        ClearPaintDirty();
        const Color fill = (m_Index & 1u) != 0
            ? ResolveColor(ColorToken::ControlBackground)
            : ResolveColor(ColorToken::SecondarySurface);
        context.DrawRect(m_Geometry, fill);
    }
private:
    size_t m_Index = 0;
};

DirtyRegionScenarioResult RunScenario(
    const char* name,
    uint32_t iterations,
    const std::function<void(VirtualList&, uint32_t)>& mutate)
{
    DirtyRegionScenarioResult result{};
    result.name = name;
    result.iterations = iterations;

    auto list = MakeListView();
    list->SetItemHeight(24.0f);
    list->SetOverscan(4);
    list->SetItemCount(2000);
    list->SetItemFactory([](size_t index, std::shared_ptr<Widget> recycled) {
        auto row = std::dynamic_pointer_cast<BenchRow>(recycled);
        if (!row) {
            row = std::make_shared<BenchRow>(index);
        } else {
            row->Rebind(index);
        }
        return row;
    });

    const Rect viewport{ 0.0f, 0.0f, 420.0f, 640.0f };
    UIDirtyRegionTracker::Get().Reset();
    UIDirtyRegionTracker::Get().MarkFullDirty();

    // Warm layout/paint.
    list->Measure({ viewport.width, viewport.height });
    list->Arrange(viewport);
    {
        PaintContext paint;
        paint.Clear();
        list->Paint(paint);
        (void)HashDrawCommands(paint.GetCommands());
    }

    double paintAccum = 0.0;
    double frameAccum = 0.0;
    float covAccum = 0.0f;
    float areaAccum = 0.0f;
    uint32_t regionAccum = 0;
    uint32_t reuseHits = 0;
    uint32_t lastCmds = 0;
    uint64_t lastHash = 0;

    for (uint32_t i = 0; i < iterations; ++i) {
        mutate(*list, i);

        const auto t0 = clock::now();
        list->Measure({ viewport.width, viewport.height });
        list->Arrange(viewport);
        const auto dirty = UIDirtyRegionTracker::Get().ConsumeMerged(viewport);
        covAccum += dirty.coverage;
        areaAccum += dirty.dirtyAreaPx;
        regionAccum += dirty.regionCount;

        PaintContext paint;
        paint.Clear();
        paint.SetPaintRetentionEnabled(true);
        const auto tPaint0 = clock::now();
        list->Paint(paint);
        const auto tPaint1 = clock::now();
        const uint64_t hash = HashDrawCommands(paint.GetCommands());
        if (hash != 0 && hash == lastHash) {
            ++reuseHits;
        }
        lastHash = hash;
        lastCmds = static_cast<uint32_t>(paint.CommandCount());
        const auto t1 = clock::now();

        paintAccum += Micros(tPaint0, tPaint1);
        frameAccum += Micros(t0, t1);
        (void)UIRepaintGate::ConsumeNeedsPaint();
        (void)UIRepaintGate::ConsumeNeedsLayout();
    }

    const double n = static_cast<double>((std::max)(1u, iterations));
    result.avgDirtyCoverage = covAccum / static_cast<float>(iterations);
    result.avgDirtyAreaPx = areaAccum / static_cast<float>(iterations);
    result.avgDirtyRegions = regionAccum / iterations;
    result.avgPaintMicros = paintAccum / n;
    result.avgDrawgenMicros = 0.0; // headless list paint only; adapter drawgen covered in editor profile
    result.avgFrameMicros = frameAccum / n;
    result.geometryReuseHits = reuseHits;
    result.paintCommands = lastCmds;
    result.drawCommands = lastCmds;
    return result;
}

} // namespace

KindUIDirtyRegionReport RunKindUIDirtyRegionBenchmark(uint32_t iterations) {
    KindUIDirtyRegionReport report{};

    report.scenarios.push_back(RunScenario("selection", iterations,
        [](VirtualList& list, uint32_t i) {
            list.SetSelectedIndex(i % list.GetItemCount());
        }));

    report.scenarios.push_back(RunScenario("hover-scroll", iterations,
        [](VirtualList& list, uint32_t i) {
            list.SetScrollOffset(static_cast<float>((i * 37u) % 800u));
        }));

    report.scenarios.push_back(RunScenario("property-rebind", iterations,
        [](VirtualList& list, uint32_t i) {
            // Force window move + rebind via scroll that changes visible range.
            list.SetScrollOffset(static_cast<float>(i) * 24.0f);
            list.InvalidateModel();
            list.SetItemCount(2000);
            list.SetItemFactory([](size_t index, std::shared_ptr<Widget> recycled) {
                auto row = std::dynamic_pointer_cast<BenchRow>(recycled);
                if (!row) {
                    row = std::make_shared<BenchRow>(index);
                } else {
                    row->Rebind(index);
                }
                return row;
            });
        }));

    // Small flex tree: expand-like invalidate of one label.
    {
        DirtyRegionScenarioResult result{};
        result.name = "expansion-label";
        result.iterations = iterations;
        auto root = std::make_shared<Flex>(FlexDirection::Column);
        std::vector<std::shared_ptr<Label>> labels;
        for (int i = 0; i < 40; ++i) {
            auto label = std::make_shared<Label>("Row " + std::to_string(i));
            root->AddChild(label);
            labels.push_back(label);
        }
        const Rect viewport{ 0.0f, 0.0f, 400.0f, 600.0f };
        UIDirtyRegionTracker::Get().Reset();
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);

        double paintAccum = 0.0;
        double frameAccum = 0.0;
        float covAccum = 0.0f;
        uint32_t regionAccum = 0;
        uint32_t reuseHits = 0;
        uint64_t lastHash = 0;

        for (uint32_t i = 0; i < iterations; ++i) {
            labels[i % labels.size()]->SetText("Row " + std::to_string(i));
            const auto t0 = clock::now();
            root->Measure({ viewport.width, viewport.height });
            root->Arrange(viewport);
            const auto dirty = UIDirtyRegionTracker::Get().ConsumeMerged(viewport);
            covAccum += dirty.coverage;
            regionAccum += dirty.regionCount;
            PaintContext paint;
            paint.Clear();
            paint.SetPaintRetentionEnabled(true);
            const auto tp0 = clock::now();
            root->PaintSubtree(paint);
            const auto tp1 = clock::now();
            const uint64_t hash = HashDrawCommands(paint.GetCommands());
            if (hash != 0 && hash == lastHash) {
                ++reuseHits;
            }
            lastHash = hash;
            result.paintCommands = static_cast<uint32_t>(paint.CommandCount());
            const auto t1 = clock::now();
            paintAccum += Micros(tp0, tp1);
            frameAccum += Micros(t0, t1);
            (void)UIRepaintGate::ConsumeNeedsPaint();
            (void)UIRepaintGate::ConsumeNeedsLayout();
        }
        const double n = static_cast<double>((std::max)(1u, iterations));
        result.avgDirtyCoverage = covAccum / static_cast<float>(iterations);
        result.avgDirtyRegions = regionAccum / iterations;
        result.avgPaintMicros = paintAccum / n;
        result.avgFrameMicros = frameAccum / n;
        result.geometryReuseHits = reuseHits;
        result.drawCommands = result.paintCommands;
        report.scenarios.push_back(std::move(result));
    }

    std::ostringstream summary;
    summary << "KindUI dirty-region bench:\n";
    for (const auto& s : report.scenarios) {
        summary << "  " << s.name
                << " cov=" << s.avgDirtyCoverage
                << " regions=" << s.avgDirtyRegions
                << " paint=" << static_cast<uint64_t>(s.avgPaintMicros) << "us"
                << " frame=" << static_cast<uint64_t>(s.avgFrameMicros) << "us"
                << " reuseHits=" << s.geometryReuseHits
                << "/" << s.iterations
                << " cmds=" << s.paintCommands
                << "\n";
    }
    report.summary = summary.str();
    return report;
}

} // namespace we::runtime::kindui
