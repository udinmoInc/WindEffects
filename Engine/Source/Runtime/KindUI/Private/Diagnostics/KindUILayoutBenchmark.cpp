// ==============================================================================
// WindEffects — KindUI — KindUILayoutBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUILayoutBenchmark.h"
#include "KindUI/Core/LayoutIncremental.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/UI/CollapsibleGroup.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"
#include "KindUI/UI/VirtualList.h"
#include "KindUI/Theme/ThemeAccess.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <sstream>
#include <vector>

namespace we::runtime::kindui {

namespace {

using clock = std::chrono::steady_clock;

double Micros(clock::time_point a, clock::time_point b) {
    return static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(b - a).count());
}

class BenchRow final : public Widget {
public:
    explicit BenchRow(size_t index = 0) : m_Index(index) {
        m_Label = std::make_shared<Label>("Item " + std::to_string(index));
        AddChild(m_Label);
    }
    void Rebind(size_t index) {
        m_Index = index;
        if (m_Label) {
            m_Label->SetText("Item " + std::to_string(index));
        }
        InvalidatePaint();
    }
    void SetSelected(bool selected) {
        if (m_Selected == selected) {
            return;
        }
        m_Selected = selected;
        InvalidatePaint();
    }
    Size Measure(const Size& availableSize) override {
        if (CanSkipMeasure(availableSize)) {
            return m_DesiredSize;
        }
        Size labelSize{};
        if (m_Label) {
            labelSize = MeasureChild(m_Label, availableSize);
        }
        m_DesiredSize = Size{ availableSize.width, (std::max)(24.0f, labelSize.height) };
        NoteMeasureCache(availableSize);
        return m_DesiredSize;
    }
    void Arrange(const Rect& allottedRect) override {
        if (CanSkipArrange(allottedRect)) {
            return;
        }
        CommitGeometry(allottedRect);
        ClearLayoutDirty();
        if (m_Label) {
            ArrangeChild(m_Label, allottedRect);
        }
    }
    void Paint(PaintContext& context) override {
        ClearPaintDirty();
        const Color fill = m_Selected
            ? ResolveColor(ColorToken::SelectionHighlight)
            : ((m_Index & 1u) != 0
                ? ResolveColor(ColorToken::ControlBackground)
                : ResolveColor(ColorToken::SecondarySurface));
        context.DrawRect(m_Geometry, fill);
        if (m_Label) {
            m_Label->PaintSubtree(context);
        }
    }
private:
    size_t m_Index = 0;
    bool m_Selected = false;
    std::shared_ptr<Label> m_Label;
};

struct Accumulators {
    double layoutUs = 0.0;
    double frameUs = 0.0;
    uint64_t measureRan = 0;
    uint64_t measureSkipped = 0;
    uint64_t arrangeRan = 0;
    uint64_t arrangeSkipped = 0;
    uint32_t fullPasses = 0;
};

void RunLayoutPass(const std::shared_ptr<Widget>& root, const Rect& viewport, Accumulators& acc) {
    LayoutIncrementalStats::ResetCurrent();
    ++LayoutIncrementalStats::Current().fullLayoutPasses;
    const auto t0 = clock::now();
    root->Measure(Size{ viewport.width, viewport.height });
    root->Arrange(viewport);
    root->ClearSubtreeLayoutDirty();
    const auto t1 = clock::now();
    acc.layoutUs += Micros(t0, t1);

    PaintContext paint;
    paint.Clear();
    paint.SetPaintRetentionEnabled(true);
    root->PaintSubtree(paint);
    const auto t2 = clock::now();
    acc.frameUs += Micros(t0, t2);

    const auto& s = LayoutIncrementalStats::Current();
    acc.measureRan += s.measureRan;
    acc.measureSkipped += s.measureSkipped;
    acc.arrangeRan += s.arrangeRan;
    acc.arrangeSkipped += s.arrangeSkipped;
    acc.fullPasses += s.fullLayoutPasses;
    (void)UIRepaintGate::ConsumeNeedsPaint();
    (void)UIRepaintGate::ConsumeNeedsLayout();
}

LayoutScenarioResult Finish(
    const char* name,
    uint32_t iterations,
    const Accumulators& acc)
{
    LayoutScenarioResult result{};
    result.name = name;
    result.iterations = iterations;
    const double n = static_cast<double>((std::max)(1u, iterations));
    result.avgMeasureArrangeMicros = acc.layoutUs / n;
    result.avgFrameMicros = acc.frameUs / n;
    result.avgMeasureRan = static_cast<uint32_t>(acc.measureRan / static_cast<uint64_t>(iterations));
    result.avgMeasureSkipped = static_cast<uint32_t>(acc.measureSkipped / static_cast<uint64_t>(iterations));
    result.avgArrangeRan = static_cast<uint32_t>(acc.arrangeRan / static_cast<uint64_t>(iterations));
    result.avgArrangeSkipped = static_cast<uint32_t>(acc.arrangeSkipped / static_cast<uint64_t>(iterations));
    result.fullLayoutPasses = acc.fullPasses;
    result.affectedWidgetsApprox = result.avgMeasureRan + result.avgArrangeRan;
    return result;
}

std::shared_ptr<Column> MakePropertyTree(std::vector<std::shared_ptr<Label>>& outLabels, int rows) {
    auto root = std::make_shared<Column>();
    root->Gap(4.0f);
    outLabels.clear();
    outLabels.reserve(static_cast<size_t>(rows));
    for (int i = 0; i < rows; ++i) {
        auto label = std::make_shared<Label>("Property " + std::to_string(i));
        outLabels.push_back(label);
        root->AddChild(label);
    }
    return root;
}

} // namespace

KindUILayoutReport RunKindUILayoutBenchmark(uint32_t iterations) {
    KindUILayoutReport report{};
    const Rect viewport{ 0.0f, 0.0f, 480.0f, 720.0f };

    // --- Property text change (single leaf) ---
    {
        std::vector<std::shared_ptr<Label>> labels;
        auto root = MakePropertyTree(labels, 120);
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        root->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            labels[i % labels.size()]->SetText("Prop " + std::to_string(i));
            RunLayoutPass(root, viewport, acc);
        }
        report.scenarios.push_back(Finish("property_change", iterations, acc));
    }

    // --- Selection (paint-mostly; layout when list rebinds selection chrome height) ---
    {
        auto list = MakeListView();
        list->SetItemHeight(24.0f);
        list->SetOverscan(4);
        list->SetItemCount(4000);
        list->SetItemFactory([&](size_t index, std::shared_ptr<Widget> recycled) {
            auto row = std::dynamic_pointer_cast<BenchRow>(recycled);
            if (!row) {
                row = std::make_shared<BenchRow>(index);
            } else {
                row->Rebind(index);
            }
            return row;
        });
        list->Measure({ viewport.width, viewport.height });
        list->Arrange(viewport);
        list->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            list->SetSelectedIndex(static_cast<size_t>(i % 4000));
            RunLayoutPass(list, viewport, acc);
        }
        report.scenarios.push_back(Finish("selection", iterations, acc));
    }

    // --- Expand / collapse ---
    {
        auto root = std::make_shared<Column>();
        std::vector<std::shared_ptr<CollapsibleGroup>> groups;
        for (int g = 0; g < 24; ++g) {
            auto group = std::make_shared<CollapsibleGroup>("Section " + std::to_string(g));
            for (int r = 0; r < 8; ++r) {
                group->AddContentChild(std::make_shared<Label>("Row " + std::to_string(r)));
            }
            group->SetExpanded(g % 2 == 0);
            groups.push_back(group);
            root->AddChild(group);
        }
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        root->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            auto& g = groups[i % groups.size()];
            g->SetExpanded(!g->IsExpanded());
            RunLayoutPass(root, viewport, acc);
        }
        report.scenarios.push_back(Finish("expand_collapse", iterations, acc));
    }

    // --- Scrolling virtual list ---
    {
        auto list = MakeListView();
        list->SetItemHeight(24.0f);
        list->SetOverscan(4);
        list->SetItemCount(50000);
        list->SetItemFactory([](size_t index, std::shared_ptr<Widget> recycled) {
            auto row = std::dynamic_pointer_cast<BenchRow>(recycled);
            if (!row) {
                row = std::make_shared<BenchRow>(index);
            } else {
                row->Rebind(index);
            }
            return row;
        });
        list->Measure({ viewport.width, viewport.height });
        list->Arrange(viewport);
        list->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            list->SetScrollOffset(static_cast<float>((i * 37u) % 20000u) * 24.0f);
            RunLayoutPass(list, viewport, acc);
        }
        report.scenarios.push_back(Finish("scroll", iterations, acc));
    }

    // --- Resize ---
    {
        std::vector<std::shared_ptr<Label>> labels;
        auto root = MakePropertyTree(labels, 80);
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        root->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            const float w = viewport.width + static_cast<float>(i % 40);
            const float h = viewport.height + static_cast<float>((i * 3) % 50);
            const Rect r{ 0.0f, 0.0f, w, h };
            LayoutIncrementalStats::ResetCurrent();
            ++LayoutIncrementalStats::Current().fullLayoutPasses;
            const auto t0 = clock::now();
            root->Measure(Size{ w, h });
            root->Arrange(r);
            root->ClearSubtreeLayoutDirty();
            const auto t1 = clock::now();
            acc.layoutUs += Micros(t0, t1);
            PaintContext paint;
            paint.Clear();
            root->PaintSubtree(paint);
            const auto t2 = clock::now();
            acc.frameUs += Micros(t0, t2);
            const auto& s = LayoutIncrementalStats::Current();
            acc.measureRan += s.measureRan;
            acc.measureSkipped += s.measureSkipped;
            acc.arrangeRan += s.arrangeRan;
            acc.arrangeSkipped += s.arrangeSkipped;
            acc.fullPasses += s.fullLayoutPasses;
        }
        report.scenarios.push_back(Finish("resize", iterations, acc));
    }

    // --- Large tree warm + leaf invalidate ---
    {
        std::vector<std::shared_ptr<Label>> labels;
        auto root = MakePropertyTree(labels, 2000);
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        root->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            labels[i % labels.size()]->SetText("L" + std::to_string(i));
            RunLayoutPass(root, viewport, acc);
        }
        report.scenarios.push_back(Finish("large_tree_leaf", iterations, acc));
    }

    // --- Full-tree baseline (invalidate every leaf so MeasureChild cannot skip) ---
    {
        std::vector<std::shared_ptr<Label>> labels;
        auto root = MakePropertyTree(labels, 2000);
        root->Measure({ viewport.width, viewport.height });
        root->Arrange(viewport);
        root->ClearSubtreeLayoutDirty();
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            for (auto& label : labels) {
                label->InvalidateLayout();
            }
            RunLayoutPass(root, viewport, acc);
        }
        report.scenarios.push_back(Finish("large_tree_full", iterations, acc));
    }

    std::ostringstream oss;
    oss << "KindUI incremental layout bench (" << iterations << " iters/scenario):\n";
    for (const auto& s : report.scenarios) {
        oss << "  " << s.name
            << " layout=" << static_cast<uint64_t>(s.avgMeasureArrangeMicros) << "us"
            << " frame=" << static_cast<uint64_t>(s.avgFrameMicros) << "us"
            << " meas=" << s.avgMeasureRan << "/" << (s.avgMeasureRan + s.avgMeasureSkipped)
            << " arr=" << s.avgArrangeRan << "/" << (s.avgArrangeRan + s.avgArrangeSkipped)
            << " full=" << s.fullLayoutPasses
            << " affected~" << s.affectedWidgetsApprox
            << "\n";
    }
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::kindui
