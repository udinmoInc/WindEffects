// ==============================================================================
// WindEffects — KindUI — KindUITextBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUITextBenchmark.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIStateChange.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"
#include "KindUI/UI/VirtualList.h"
#include "KindUI/Theme/ThemeAccess.h"
#include "Rendering/TextUIService.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "KindUI/Core/TextMetrics.h"

#include <chrono>
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
    double measureUs = 0.0;
    double layoutUs = 0.0;
    double drawgenUs = 0.0;
    double frameUs = 0.0;
    uint64_t layoutHits = 0;
    uint64_t layoutMisses = 0;
    uint64_t geomHits = 0;
    uint64_t geomMisses = 0;
    uint64_t glyphs = 0;
    uint64_t textDraws = 0;
    uint64_t atlasUploads = 0;
    uint64_t batches = 0;
    uint64_t textureSwitches = 0;
    uint64_t layoutCacheBytes = 0;
};

TextScenarioResult Finish(const char* name, uint32_t iterations, const Accumulators& acc) {
    TextScenarioResult r{};
    r.name = name;
    r.iterations = iterations;
    const double n = static_cast<double>((std::max)(1u, iterations));
    r.avgMeasureUs = acc.measureUs / n;
    r.avgLayoutUs = acc.layoutUs / n;
    r.avgDrawgenUs = acc.drawgenUs / n;
    r.avgFrameUs = acc.frameUs / n;
    r.avgLayoutHits = static_cast<uint32_t>(acc.layoutHits / static_cast<uint64_t>(iterations));
    r.avgLayoutMisses = static_cast<uint32_t>(acc.layoutMisses / static_cast<uint64_t>(iterations));
    r.avgGeomHits = static_cast<uint32_t>(acc.geomHits / static_cast<uint64_t>(iterations));
    r.avgGeomMisses = static_cast<uint32_t>(acc.geomMisses / static_cast<uint64_t>(iterations));
    r.avgGlyphs = static_cast<uint32_t>(acc.glyphs / static_cast<uint64_t>(iterations));
    r.avgTextDraws = static_cast<uint32_t>(acc.textDraws / static_cast<uint64_t>(iterations));
    r.avgAtlasUploads = static_cast<uint32_t>(acc.atlasUploads / static_cast<uint64_t>(iterations));
    r.avgBatches = static_cast<uint32_t>(acc.batches / static_cast<uint64_t>(iterations));
    r.avgTextureSwitches = static_cast<uint32_t>(acc.textureSwitches / static_cast<uint64_t>(iterations));
    r.avgLayoutCacheBytes = acc.layoutCacheBytes / static_cast<uint64_t>(iterations);
    return r;
}

void RecordTextStats(TextUIService* svc, Accumulators& acc) {
    if (!svc) {
        return;
    }
    const auto& s = svc->CurrentFrameStats();
    acc.layoutHits += s.layoutHits;
    acc.layoutMisses += s.layoutMisses;
    acc.geomHits += s.geometryCacheHits;
    acc.geomMisses += s.geometryCacheMisses;
    acc.glyphs += s.glyphsEmitted;
    acc.textDraws += s.textDraws;
    acc.atlasUploads += s.atlasUploads;
    acc.layoutCacheBytes += s.layoutCacheBytes;
}

class TextBenchRow final : public Widget {
public:
    explicit TextBenchRow(size_t index = 0) : m_Index(index) {
        m_Label = std::make_shared<Label>("Row " + std::to_string(index));
        AddChild(m_Label);
    }
    void Rebind(size_t index) {
        m_Index = index;
        if (m_Label) {
            m_Label->SetText("Row " + std::to_string(index));
        }
        UIStateChangeGate::Post(*this, StateChangeKind::Selection);
    }
    Size Measure(const Size& availableSize) override {
        m_DesiredSize = Size{ availableSize.width, 24.0f };
        return m_DesiredSize;
    }
    void Arrange(const Rect& allottedRect) override {
        CommitGeometry(allottedRect);
        ClearLayoutDirty();
        if (m_Label) {
            m_Label->Arrange(allottedRect);
        }
    }
    void Paint(PaintContext& context) override {
        ClearPaintDirty();
        context.DrawRect(m_Geometry, ResolveColor(ColorToken::ControlBackground));
        if (m_Label) {
            m_Label->PaintSubtree(context);
        }
    }
private:
    size_t m_Index = 0;
    std::shared_ptr<Label> m_Label;
};

/// Headless drawgen: measure+layout widgets then emit text geometry through TextUIService.
void EmitPaintPass(
    TextUIService* textService,
    const std::shared_ptr<Widget>& root,
    const Rect& viewport,
    Accumulators& acc)
{
    const auto t0 = clock::now();
    root->Measure(Size{ viewport.width, viewport.height });
    root->Arrange(viewport);
    const auto t1 = clock::now();
    acc.layoutUs += Micros(t0, t1);

    PaintContext paint;
    paint.Clear();
    paint.SetPaintRetentionEnabled(true);
    if (textService) {
        paint.SetTextUIService(textService);
        textService->BeginFrame();
    }
    root->PaintSubtree(paint);

    std::vector<UIVertex2> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(4096);
    indices.reserve(6144);
    uint32_t batches = 0;
    uint32_t texSwitches = 0;
    we::rhi::RHIDescriptorSetHandle lastTex = we::rhi::RHIDescriptorSetHandle::Invalid;
    bool hasTex = false;

    const auto t2 = clock::now();
    for (const auto& cmd : paint.GetCommands()) {
        if (cmd.type != DrawCommandType::Text || !textService) {
            continue;
        }
        we::rhi::RHIDescriptorSetHandle tex{};
        UIRenderBatch info{};
        if (textService->GenerateTextGeometry(cmd, vertices, indices, tex, &info)) {
            if (hasTex && tex != lastTex) {
                ++texSwitches;
            }
            lastTex = tex;
            hasTex = true;
            ++batches;
        }
    }
    const auto t3 = clock::now();
    acc.drawgenUs += Micros(t2, t3);
    acc.frameUs += Micros(t0, t3);
    acc.batches += batches;
    acc.textureSwitches += texSwitches;
    RecordTextStats(textService, acc);
    (void)UIRepaintGate::ConsumeNeedsPaint();
    (void)UIRepaintGate::ConsumeNeedsLayout();
}

} // namespace

KindUITextReport RunKindUITextBenchmark(TextUIService* textService, uint32_t iterations) {
    KindUITextReport report{};
    const Rect viewport{ 0.0f, 0.0f, 640.0f, 720.0f };
    TextUIService* liveService = textService;

    // --- Cold measure (unique strings) ---
    {
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            const std::string text = "ColdUnique_" + std::to_string(i) + "_Unicode_日本語_✓";
            const auto t0 = clock::now();
            const float w = TextMetrics::MeasureWidth(text, 13.0f, false);
            const auto t1 = clock::now();
            acc.measureUs += Micros(t0, t1);
            (void)w;
        }
        report.scenarios.push_back(Finish("cold_measure", iterations, acc));
    }

    // --- Warm measure (same string) ---
    {
        Accumulators acc{};
        const std::string text = "WarmRepeatedLabel_PropertyValue_42";
        (void)TextMetrics::MeasureWidth(text, 13.0f, false); // prime
        for (uint32_t i = 0; i < iterations; ++i) {
            const auto t0 = clock::now();
            const float w = TextMetrics::MeasureWidth(text, 13.0f, (i & 1u) == 0);
            const auto t1 = clock::now();
            acc.measureUs += Micros(t0, t1);
            (void)w;
        }
        report.scenarios.push_back(Finish("warm_measure", iterations, acc));
    }

    // --- Many labels tree ---
    {
        auto root = std::make_shared<Column>();
        root->Gap(2.0f);
        std::vector<std::shared_ptr<Label>> labels;
        for (int i = 0; i < 200; ++i) {
            auto label = std::make_shared<Label>("Label " + std::to_string(i));
            labels.push_back(label);
            root->AddChild(label);
        }
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            EmitPaintPass(liveService, root, viewport, acc);
        }
        report.scenarios.push_back(Finish("many_labels", iterations, acc));
    }

    // --- Repeated identical text (geometry cache warm) ---
    {
        auto root = std::make_shared<Column>();
        for (int i = 0; i < 80; ++i) {
            root->AddChild(std::make_shared<Label>("SameText"));
        }
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            EmitPaintPass(liveService, root, viewport, acc);
        }
        report.scenarios.push_back(Finish("repeated_text", iterations, acc));
    }

    // --- Selection / property updates ---
    {
        auto root = std::make_shared<Column>();
        std::vector<std::shared_ptr<Label>> labels;
        for (int i = 0; i < 100; ++i) {
            auto label = std::make_shared<Label>("Prop " + std::to_string(i));
            labels.push_back(label);
            root->AddChild(label);
        }
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            labels[i % labels.size()]->SetText("Prop " + std::to_string(i));
            labels[(i + 3) % labels.size()]->SetSelected((i & 1u) != 0);
            EmitPaintPass(liveService, root, viewport, acc);
        }
        report.scenarios.push_back(Finish("selection_property", iterations, acc));
    }

    // --- Scrolling virtual list ---
    {
        auto list = MakeListView();
        list->SetItemHeight(24.0f);
        list->SetOverscan(4);
        list->SetItemCount(5000);
        list->SetItemFactory([](size_t index, std::shared_ptr<Widget> recycled) {
            auto row = std::dynamic_pointer_cast<TextBenchRow>(recycled);
            if (!row) {
                row = std::make_shared<TextBenchRow>(index);
            } else {
                row->Rebind(index);
            }
            return row;
        });
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            list->SetScrollOffset(static_cast<float>((i * 37u) % 2000u) * 24.0f);
            EmitPaintPass(liveService, list, viewport, acc);
        }
        report.scenarios.push_back(Finish("scroll", iterations, acc));
    }

    // --- Mixed icon + text (paint commands only; icons need GPU service) ---
    {
        auto root = std::make_shared<Column>();
        for (int i = 0; i < 60; ++i) {
            root->AddChild(std::make_shared<Label>("IconRow " + std::to_string(i)));
        }
        Accumulators acc{};
        for (uint32_t i = 0; i < iterations; ++i) {
            EmitPaintPass(liveService, root, viewport, acc);
        }
        report.scenarios.push_back(Finish("mixed_labels", iterations, acc));
    }

    std::ostringstream oss;
    oss << "KindUI text path bench (" << iterations << " iters/scenario):\n";
    for (const auto& s : report.scenarios) {
        oss << "  " << s.name
            << " measure=" << static_cast<uint64_t>(s.avgMeasureUs) << "us"
            << " layout=" << static_cast<uint64_t>(s.avgLayoutUs) << "us"
            << " drawgen=" << static_cast<uint64_t>(s.avgDrawgenUs) << "us"
            << " frame=" << static_cast<uint64_t>(s.avgFrameUs) << "us"
            << " layHit=" << s.avgLayoutHits << "/" << (s.avgLayoutHits + s.avgLayoutMisses)
            << " geomHit=" << s.avgGeomHits << "/" << (s.avgGeomHits + s.avgGeomMisses)
            << " glyphs=" << s.avgGlyphs
            << " draws=" << s.avgTextDraws
            << " batches=" << s.avgBatches
            << " texSw=" << s.avgTextureSwitches
            << " cacheB=" << s.avgLayoutCacheBytes
            << "\n";
    }
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::kindui
