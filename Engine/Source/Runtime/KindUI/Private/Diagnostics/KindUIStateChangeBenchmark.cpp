// ==============================================================================

// WindEffects — KindUI — KindUIStateChangeBenchmark

// Internal implementation for the KindUI module.

//

// Copyright (c) 2026 WindEffects. All rights reserved.

// This file is part of WindEffects Engine and is governed by the

// WindEffects Engine EULA (see Legal/EULA.md at the repository root).

// ==============================================================================

#include "KindUI/Diagnostics/KindUIStateChangeBenchmark.h"

#include "KindUI/Core/LayoutIncremental.h"

#include "KindUI/Core/PaintContext.h"

#include "KindUI/Core/UIRepaintGate.h"

#include "KindUI/Core/UIStateChange.h"

#include "KindUI/UI/CollapsibleGroup.h"

#include "KindUI/UI/Flex.h"

#include "KindUI/UI/Label.h"

#include "KindUI/UI/VirtualList.h"

#include "KindUI/Theme/ThemeAccess.h"



#include <chrono>

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



struct Accumulators {

    double updateUs = 0.0;

    double frameUs = 0.0;

    uint64_t notifications = 0;

    uint64_t deduped = 0;

    uint64_t widgetsPaint = 0;

    uint64_t widgetsLayout = 0;

    uint64_t gatePaintArms = 0;

    uint64_t gateLayoutArms = 0;

    uint64_t transactions = 0;

    uint64_t measureRan = 0;

    uint64_t arrangeRan = 0;

    uint64_t layoutPasses = 0;

};



void RunFramePass(const std::shared_ptr<Widget>& root, const Rect& viewport, Accumulators& acc) {

    LayoutIncrementalStats::ResetCurrent();

    ++LayoutIncrementalStats::Current().fullLayoutPasses;



    const auto t0 = clock::now();

    if (UIRepaintGate::ConsumeNeedsLayout()) {

        root->Measure(Size{ viewport.width, viewport.height });

        root->Arrange(viewport);

        root->ClearSubtreeLayoutDirty();

    }

    PaintContext paint;

    paint.Clear();

    paint.SetPaintRetentionEnabled(true);

    if (UIRepaintGate::ConsumeNeedsPaint()) {

        root->PaintSubtree(paint);

    }

    const auto t1 = clock::now();

    acc.frameUs += Micros(t0, t1);



    const auto& s = LayoutIncrementalStats::Current();

    acc.measureRan += s.measureRan;

    acc.arrangeRan += s.arrangeRan;

    acc.layoutPasses += s.fullLayoutPasses;

}



StateChangeScenarioResult Finish(

    const char* name,

    uint32_t iterations,

    const Accumulators& acc)

{

    StateChangeScenarioResult result{};

    result.name = name;

    result.iterations = iterations;

    const double n = static_cast<double>((std::max)(1u, iterations));

    result.avgUpdateMicros = acc.updateUs / n;

    result.avgFrameMicros = acc.frameUs / n;

    result.avgNotifications = static_cast<uint32_t>(acc.notifications / static_cast<uint64_t>(iterations));

    result.avgDeduped = static_cast<uint32_t>(acc.deduped / static_cast<uint64_t>(iterations));

    result.avgWidgetsPaint = static_cast<uint32_t>(acc.widgetsPaint / static_cast<uint64_t>(iterations));

    result.avgWidgetsLayout = static_cast<uint32_t>(acc.widgetsLayout / static_cast<uint64_t>(iterations));

    result.avgGatePaintArms = static_cast<uint32_t>(acc.gatePaintArms / static_cast<uint64_t>(iterations));

    result.avgGateLayoutArms = static_cast<uint32_t>(acc.gateLayoutArms / static_cast<uint64_t>(iterations));

    result.avgTransactions = static_cast<uint32_t>(acc.transactions / static_cast<uint64_t>(iterations));

    result.avgMeasureRan = static_cast<uint32_t>(acc.measureRan / static_cast<uint64_t>(iterations));

    result.avgArrangeRan = static_cast<uint32_t>(acc.arrangeRan / static_cast<uint64_t>(iterations));

    result.avgLayoutPasses = static_cast<uint32_t>(acc.layoutPasses / static_cast<uint64_t>(iterations));

    return result;

}



void RecordStateStats(Accumulators& acc) {

    const auto& s = UIStateChangeGate::CurrentFrameStats();

    acc.notifications += s.notifications;

    acc.deduped += s.deduped;

    acc.widgetsPaint += s.widgetsPaint;

    acc.widgetsLayout += s.widgetsLayout;

    acc.gatePaintArms += s.gatePaintArms;

    acc.gateLayoutArms += s.gateLayoutArms;

    acc.transactions += s.transactions;

}



class HoverLeaf final : public Widget {

public:

    Size Measure(const Size& availableSize) override {

        m_DesiredSize = Size{ availableSize.width, 28.0f };

        return m_DesiredSize;

    }

    void Arrange(const Rect& allottedRect) override {

        CommitGeometry(allottedRect);

        ClearLayoutDirty();

    }

    void Paint(PaintContext& context) override {

        ClearPaintDirty();

        Color fill = ResolveColor(ColorToken::ControlBackground);

        if (IsHovered()) {

            fill = ResolveColor(ColorToken::SelectionHighlight);

        }

        context.DrawRect(m_Geometry, fill);

    }

};



std::shared_ptr<Widget> MakeHoverChain(int depth) {

    std::shared_ptr<Widget> root = std::make_shared<HoverLeaf>();

    std::shared_ptr<Widget> current = root;

    for (int i = 1; i < depth; ++i) {

        auto child = std::make_shared<HoverLeaf>();

        current->AddChild(child);

        current = child;

    }

    return root;

}



class FocusStub final : public Widget {

public:

    explicit FocusStub(std::string label) : m_Label(std::move(label)) {

        SetFocusable(true);

    }

    Size Measure(const Size& availableSize) override {

        m_DesiredSize = Size{ availableSize.width, 32.0f };

        return m_DesiredSize;

    }

    void Arrange(const Rect& allottedRect) override {

        CommitGeometry(allottedRect);

        ClearLayoutDirty();

    }

    void Paint(PaintContext& context) override {

        ClearPaintDirty();

        Color fill = IsFocused()

            ? ResolveColor(ColorToken::SelectionHighlight)

            : ResolveColor(ColorToken::ControlBackground);

        context.DrawRect(m_Geometry, fill);

    }

private:

    std::string m_Label;

};



class BenchRow final : public Widget {

public:

    explicit BenchRow(size_t index = 0) : m_Index(index) {}

    void Rebind(size_t index) {

        m_Index = index;

        UIStateChangeGate::Post(*this, StateChangeKind::Selection);

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

        const Color fill = m_Selected

            ? ResolveColor(ColorToken::SelectionHighlight)

            : ((m_Index & 1u) != 0

                ? ResolveColor(ColorToken::ControlBackground)

                : ResolveColor(ColorToken::SecondarySurface));

        context.DrawRect(m_Geometry, fill);

    }

    void SetRowSelected(bool selected) {

        if (m_Selected == selected) {

            return;

        }

        m_Selected = selected;

        UIStateChangeGate::Post(*this, StateChangeKind::Selection);

    }

private:

    size_t m_Index = 0;

    bool m_Selected = false;

};



} // namespace



KindUIStateChangeReport RunKindUIStateChangeBenchmark(uint32_t iterations) {

    KindUIStateChangeReport report{};

    const Rect viewport{ 0.0f, 0.0f, 480.0f, 720.0f };



    // --- Hover chain (coalesced transaction) ---

    {

        auto root = MakeHoverChain(8);

        root->Measure({ viewport.width, viewport.height });

        root->Arrange(viewport);

        root->ClearSubtreeLayoutDirty();

        Accumulators acc{};

        for (uint32_t i = 0; i < iterations; ++i) {

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            {

                UIStateChangeGate::ScopedTransaction tx("HoverBench");

                std::shared_ptr<Widget> node = root;

                const int depth = static_cast<int>(i % 8);

                for (int d = 0; d < depth && node; ++d) {

                    node->SetHovered(true);

                    const auto& children = node->GetChildren();

                    node = children.empty() ? nullptr : children.front();

                }

                for (int d = depth; d < 8; ++d) {

                    if (node) {

                        node->SetHovered(false);

                        const auto& children = node->GetChildren();

                        node = children.empty() ? nullptr : children.front();

                    }

                }

            }

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(root, viewport, acc);

        }

        report.scenarios.push_back(Finish("hover", iterations, acc));

    }



    // --- Focus swap ---

    {

        auto a = std::make_shared<FocusStub>("A");

        auto b = std::make_shared<FocusStub>("B");

        auto root = std::make_shared<Column>();

        root->AddChild(a);

        root->AddChild(b);

        root->Measure({ viewport.width, viewport.height });

        root->Arrange(viewport);

        root->ClearSubtreeLayoutDirty();

        Accumulators acc{};

        for (uint32_t i = 0; i < iterations; ++i) {

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            UIStateChangeGate::ScopedTransaction tx("FocusBench");

            if ((i & 1u) == 0) {

                a->OnFocus();

                b->OnBlur();

            } else {

                b->OnFocus();

                a->OnBlur();

            }

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(root, viewport, acc);

        }

        report.scenarios.push_back(Finish("focus", iterations, acc));

    }



    // --- Selection (virtual list) ---

    {

        auto list = MakeListView();

        list->SetItemHeight(24.0f);

        list->SetOverscan(4);

        list->SetItemCount(4000);

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

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            list->SetSelectedIndex(static_cast<size_t>(i % 4000));

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(list, viewport, acc);

        }

        report.scenarios.push_back(Finish("selection", iterations, acc));

    }



    // --- Unselection ---

    {

        auto list = MakeListView();

        list->SetItemHeight(24.0f);

        list->SetOverscan(4);

        list->SetItemCount(1000);

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

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            list->SetSelectedIndex(static_cast<size_t>(i % 1000));

            list->ClearSelection();

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(list, viewport, acc);

        }

        report.scenarios.push_back(Finish("unselection", iterations, acc));

    }



    // --- Multi-selection ---

    {

        auto list = MakeListView();

        list->SetItemHeight(24.0f);

        list->SetOverscan(4);

        list->SetItemCount(500);

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

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            UIStateChangeGate::ScopedTransaction tx("MultiSelect");

            list->ClearSelection();

            for (uint32_t k = 0; k < 8; ++k) {

                list->SetSelectedIndex(static_cast<size_t>((i + k * 17u) % 500), true);

            }

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(list, viewport, acc);

        }

        report.scenarios.push_back(Finish("multi_selection", iterations, acc));

    }



    // --- Property (read-only toggle on leaf widgets) ---

    {

        auto root = std::make_shared<Column>();

        std::vector<std::shared_ptr<Label>> labels;

        for (int i = 0; i < 120; ++i) {

            auto label = std::make_shared<Label>("Property " + std::to_string(i));

            labels.push_back(label);

            root->AddChild(label);

        }

        root->Measure({ viewport.width, viewport.height });

        root->Arrange(viewport);

        root->ClearSubtreeLayoutDirty();

        Accumulators acc{};

        for (uint32_t i = 0; i < iterations; ++i) {

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            labels[i % labels.size()]->SetReadOnly((i & 1u) != 0);

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(root, viewport, acc);

        }

        report.scenarios.push_back(Finish("property", iterations, acc));

    }



    // --- Expansion ---

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

            UIStateChangeGate::ResetFrameStats();

            const auto t0 = clock::now();

            auto& g = groups[i % groups.size()];

            g->SetExpanded(!g->IsExpanded());

            const auto t1 = clock::now();

            acc.updateUs += Micros(t0, t1);

            RecordStateStats(acc);

            RunFramePass(root, viewport, acc);

        }

        report.scenarios.push_back(Finish("expansion", iterations, acc));

    }



    std::ostringstream oss;

    oss << "KindUI state-change bench (" << iterations << " iters/scenario):\n";

    for (const auto& s : report.scenarios) {

        oss << "  " << s.name

            << " update=" << static_cast<uint64_t>(s.avgUpdateMicros) << "us"

            << " frame=" << static_cast<uint64_t>(s.avgFrameMicros) << "us"

            << " notify=" << s.avgNotifications

            << " dedup=" << s.avgDeduped

            << " paintW=" << s.avgWidgetsPaint

            << " layoutW=" << s.avgWidgetsLayout

            << " gateP=" << s.avgGatePaintArms

            << " gateL=" << s.avgGateLayoutArms

            << " tx=" << s.avgTransactions

            << " meas=" << s.avgMeasureRan

            << " arr=" << s.avgArrangeRan

            << "\n";

    }

    report.summary = oss.str();

    return report;

}



} // namespace we::runtime::kindui

