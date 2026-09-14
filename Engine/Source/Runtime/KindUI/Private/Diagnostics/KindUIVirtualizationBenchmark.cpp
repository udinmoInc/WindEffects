// ==============================================================================
// WindEffects — KindUI — KindUIVirtualizationBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIVirtualizationBenchmark.h"
#include "KindUI/Core/InputEvents.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/UI/CompactTreeWidget.h"
#include "KindUI/UI/VirtualList.h"
#include "KindUI/Theme/ThemeAccess.h"

#include <chrono>
#include <cmath>
#include <sstream>

namespace we::runtime::kindui {

namespace {

using clock = std::chrono::steady_clock;

constexpr float kViewportW = 420.0f;
constexpr float kViewportH = 640.0f;
constexpr float kRowH = 24.0f;

/// Minimal row for recycle tests — no text/font dependencies.
class BenchRow final : public Widget {
public:
    explicit BenchRow(size_t index = 0) : m_Index(index) {}

    void Rebind(size_t index) { m_Index = index; InvalidatePaint(); }

    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ availableSize.width, kRowH };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        ClearLayoutDirty();
    }

    void Paint(PaintContext& context) override {
        ClearPaintDirty();
        // Alternate fill so command count stays proportional to active rows.
        const Color fill = (m_Index & 1u) != 0
            ? ResolveColor(ColorToken::ControlBackground)
            : ResolveColor(ColorToken::SecondarySurface);
        context.DrawRect(m_Geometry, fill);
    }

private:
    size_t m_Index = 0;
};

double MicrosBetween(clock::time_point a, clock::time_point b) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
}

VirtualizationScaleResult BenchVirtualList(uint32_t itemCount, uint32_t scrollSteps) {
    VirtualizationScaleResult result{};
    result.itemCount = itemCount;

    auto list = MakeListView();
    list->SetItemHeight(kRowH);
    list->SetOverscan(4);
    list->SetItemCount(itemCount);
    list->SetItemFactory([](size_t index, std::shared_ptr<Widget> recycled) -> std::shared_ptr<Widget> {
        auto row = std::dynamic_pointer_cast<BenchRow>(recycled);
        if (!row) {
            row = std::make_shared<BenchRow>(index);
        } else {
            row->Rebind(index);
        }
        return row;
    });

    const Rect viewport{ 0.0f, 0.0f, kViewportW, kViewportH };
    list->ResetRecycleCounters();

    {
        const auto t0 = clock::now();
        list->Measure({ kViewportW, kViewportH });
        list->Arrange(viewport);
        const auto t1 = clock::now();
        result.layoutMicros = MicrosBetween(t0, t1);
    }

    PaintContext paint;
    {
        const auto t0 = clock::now();
        paint.Clear();
        list->Paint(paint);
        const auto t1 = clock::now();
        result.paintMicros = MicrosBetween(t0, t1);
        result.paintCommands = static_cast<uint32_t>(paint.CommandCount());
        result.drawCommandPeak = result.paintCommands;
    }

    const float contentH = static_cast<float>(itemCount) * kRowH;
    const float maxScroll = (std::max)(0.0f, contentH - kViewportH);
    const uint32_t steps = (std::max)(1u, scrollSteps);
    double scrollAccum = 0.0;
    uint32_t cmdPeak = result.drawCommandPeak;

    for (uint32_t s = 0; s < steps; ++s) {
        const float t = steps == 1 ? 0.0f : static_cast<float>(s) / static_cast<float>(steps - 1);
        const auto t0 = clock::now();
        list->SetScrollOffset(maxScroll * t);
        list->Measure({ kViewportW, kViewportH });
        list->Arrange(viewport);
        list->Tick(1.0f / 60.0f);
        paint.Clear();
        list->Paint(paint);
        const auto t1 = clock::now();
        scrollAccum += MicrosBetween(t0, t1);
        cmdPeak = (std::max)(cmdPeak, static_cast<uint32_t>(paint.CommandCount()));
    }

    result.scrollFrameMicros = scrollAccum / static_cast<double>(steps);
    result.drawCommandPeak = cmdPeak;
    result.activeWidgets = static_cast<uint32_t>(list->GetActiveWidgetCount());
    result.poolSize = static_cast<uint32_t>(list->GetRecyclePool().PoolSize());
    result.createCount = list->GetRecyclePool().CreateCount();
    result.recycleCount = list->GetRecyclePool().RecycleCount();
    result.estimatedMemoryBytes =
        static_cast<uint64_t>(itemCount) * 32ull
        + static_cast<uint64_t>(result.activeWidgets + result.poolSize) * 128ull;
    result.gpuUploads = 0;

    // Explicit teardown before shared_ptr release (children/slots/pool).
    list->InvalidateModel();
    return result;
}

VirtualizationScaleResult BenchCompactTree(uint32_t itemCount, uint32_t scrollSteps) {
    VirtualizationScaleResult result{};
    result.itemCount = itemCount;

    std::vector<CompactTreeNode> nodes;
    nodes.reserve(itemCount);
    for (uint32_t i = 0; i < itemCount; ++i) {
        CompactTreeNode node{};
        node.id = "n" + std::to_string(i);
        node.label = "Node " + std::to_string(i);
        node.category = node.id;
        node.depth = 0;
        node.hasChildren = false;
        node.expanded = true;
        nodes.push_back(std::move(node));
    }

    auto tree = std::make_shared<CompactTreeWidget>();
    tree->SetItems(std::move(nodes));

    const Rect viewport{ 0.0f, 0.0f, kViewportW, kViewportH };
    {
        const auto t0 = clock::now();
        tree->Measure({ kViewportW, kViewportH });
        tree->Arrange(viewport);
        const auto t1 = clock::now();
        result.layoutMicros = MicrosBetween(t0, t1);
    }

    PaintContext paint;
    {
        const auto t0 = clock::now();
        paint.Clear();
        tree->Paint(paint);
        const auto t1 = clock::now();
        result.paintMicros = MicrosBetween(t0, t1);
        result.paintCommands = static_cast<uint32_t>(paint.CommandCount());
        result.drawCommandPeak = result.paintCommands;
    }

    const uint32_t steps = (std::max)(1u, scrollSteps);
    double scrollAccum = 0.0;
    uint32_t cmdPeak = result.drawCommandPeak;

    for (uint32_t s = 0; s < steps; ++s) {
        MouseEvent wheel{};
        wheel.type = MouseEventType::MouseWheel;
        wheel.wheelDeltaY = (s & 1u) != 0 ? 1.0f : -1.0f;
        wheel.deltaY = wheel.wheelDeltaY;
        const auto t0 = clock::now();
        tree->OnMouseWheel(wheel);
        tree->Measure({ kViewportW, kViewportH });
        tree->Arrange(viewport);
        paint.Clear();
        tree->Paint(paint);
        const auto t1 = clock::now();
        scrollAccum += MicrosBetween(t0, t1);
        cmdPeak = (std::max)(cmdPeak, static_cast<uint32_t>(paint.CommandCount()));
    }

    result.scrollFrameMicros = scrollAccum / static_cast<double>(steps);
    result.drawCommandPeak = cmdPeak;
    const auto range = tree->GetActiveRange();
    result.activeWidgets = static_cast<uint32_t>(range.count);
    result.poolSize = 0;
    result.createCount = 0;
    result.recycleCount = 0;
    result.estimatedMemoryBytes =
        static_cast<uint64_t>(itemCount) * 128ull
        + static_cast<uint64_t>(result.activeWidgets) * 64ull;
    result.gpuUploads = 0;
    return result;
}

std::string FormatScale(const char* kind, const VirtualizationScaleResult& r) {
    std::ostringstream oss;
    oss << kind << " N=" << r.itemCount
        << " active=" << r.activeWidgets
        << " pool=" << r.poolSize
        << " create=" << r.createCount
        << " recycle=" << r.recycleCount
        << " mem~=" << (r.estimatedMemoryBytes / 1024ull) << "KiB"
        << " layout=" << static_cast<uint64_t>(r.layoutMicros) << "us"
        << " paint=" << static_cast<uint64_t>(r.paintMicros) << "us"
        << " scroll=" << static_cast<uint64_t>(r.scrollFrameMicros) << "us"
        << " cmds=" << r.paintCommands
        << " cmdPeak=" << r.drawCommandPeak
        << " gpuUp=" << r.gpuUploads;
    return oss.str();
}

} // namespace

KindUIVirtualizationReport RunKindUIVirtualizationBenchmark(uint32_t scrollSteps) {
    KindUIVirtualizationReport report{};
    const uint32_t scales[] = { 100u, 1000u, 10000u, 100000u };

    std::ostringstream summary;
    summary << "KindUI virtualization bench (viewport "
            << static_cast<int>(kViewportW) << "x" << static_cast<int>(kViewportH) << "):\n";

    for (uint32_t n : scales) {
        report.listScales.push_back(BenchVirtualList(n, scrollSteps));
        summary << "  " << FormatScale("ListView", report.listScales.back()) << "\n";
        report.treeScales.push_back(BenchCompactTree(n, scrollSteps));
        summary << "  " << FormatScale("CompactTree", report.treeScales.back()) << "\n";
    }

    report.summary = summary.str();
    return report;
}

} // namespace we::runtime::kindui
