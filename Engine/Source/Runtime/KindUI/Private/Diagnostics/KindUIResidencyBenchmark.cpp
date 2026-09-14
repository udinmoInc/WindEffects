// ==============================================================================
// WindEffects — KindUI — KindUIResidencyBenchmark
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Diagnostics/KindUIResidencyBenchmark.h"

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/UIResourceResidency.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Host/IconManager.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "KindUI/UI/Flex.h"
#include "KindUI/UI/Label.h"
#include "KindUI/UI/OverlayManager.h"
#include "Rendering/TextUIService.h"

#include <algorithm>
#include <chrono>
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

ResidencyScenarioResult Finish(
    const char* name,
    uint32_t iterations,
    double frameUs,
    const UIResidencyStats& before,
    const UIResidencyStats& after)
{
    ResidencyScenarioResult r{};
    r.name = name;
    r.iterations = iterations;
    r.avgFrameMicros = frameUs / static_cast<double>((std::max)(1u, iterations));
    r.residentIcons = after.residentIconCount;
    r.residentIconGpuBytes = after.residentIconGpuBytes;
    r.residentTextGeom = after.residentTextGeomCount;
    r.residentTextGeomBytes = after.residentTextGeomCpuBytes;
    r.loads = after.loads - before.loads;
    r.uploads = after.uploads - before.uploads;
    r.evictions = after.evictions - before.evictions;
    r.cacheHits = after.cacheHits - before.cacheHits;
    r.cacheMisses = after.cacheMisses - before.cacheMisses;
    r.deferredReleases = after.deferredReleases - before.deferredReleases;
    return r;
}

void SnapshotIcons(OverlayRenderer* overlay) {
    auto& residency = UIResourceResidency::Get();
    const auto& prior = residency.Stats();
    IconManager* icons = overlay ? overlay->GetIconManager() : nullptr;
    residency.SetResidentSnapshot(
        icons ? static_cast<uint32_t>(icons->TextureCacheEntryCount()) : 0,
        icons ? icons->EstimatedGpuBytes() : 0,
        prior.residentTextGeomCount,
        prior.residentTextGeomCpuBytes,
        prior.residentGlyphCount,
        prior.residentAtlasCpuBytes);
}

void AdvanceFrame(uint64_t& frame, OverlayRenderer* overlay, uint32_t fif = 2) {
    ++frame;
    UIResourceResidency::Get().BeginFrame(frame, fif);
    if (overlay && overlay->GetTextUIService()) {
        overlay->GetTextUIService()->OnResidencyTick();
    }
    SnapshotIcons(overlay);
}

} // namespace

KindUIResidencyReport RunKindUIResidencyBenchmark(OverlayRenderer* overlay, uint32_t iterations) {
    KindUIResidencyReport report{};
    const uint32_t n = (std::max)(1u, iterations);
    auto& residency = UIResourceResidency::Get();
    residency.LoadBudgetsFromEnv();

    UIResidencyBudgets saved = residency.Budgets();
    UIResidencyBudgets tight = saved;
    tight.iconGpuBudgetBytes = 256ull * 1024ull; // 256 KiB — force icon pressure
    tight.textGeometryBudgetBytes = 64ull * 1024ull;
    tight.glyphEntryBudget = 128;
    tight.iconIdleFramesBeforeEvictable = 2;
    tight.textGeomIdleFramesBeforeEvictable = 2;
    tight.tickIntervalFrames = 1;
    tight.maxEvictionsPerTick = 16;

    IconManager* icons = overlay ? overlay->GetIconManager() : nullptr;
    TextUIService* text = overlay ? overlay->GetTextUIService() : nullptr;
    uint64_t frame = 1000;

    // --- Idle ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            if (icons) {
                icons->OnFrame(frame, overlay, nullptr);
            }
            SnapshotIcons(overlay);
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("idle", n, us, before, residency.Stats()));
    }

    // --- Icon-heavy ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        static const WindIconRef kIconHeavy[] = {
            WindIcons::Save16, WindIcons::Folder16, WindIcons::Plus16, WindIcons::Minus16,
            WindIcons::Search16, WindIcons::Settings16, WindIcons::Play16, WindIcons::Pause16,
            WindIcons::Box16, WindIcons::Bulb16, WindIcons::Brush16, WindIcons::ToolbarCamera16,
            WindIcons::Check16, WindIcons::Book16, WindIcons::Bug16, WindIcons::Ban16,
            WindIcons::Save24, WindIcons::Folder24, WindIcons::Plus24, WindIcons::Search24,
            WindIcons::Settings24, WindIcons::Play24, WindIcons::Box24, WindIcons::Check24,
            WindIcons::Brush24, WindIcons::ToolbarCamera24, WindIcons::Book24, WindIcons::Bug24,
            WindIcons::Bulb24, WindIcons::Ban24, WindIcons::Minus24, WindIcons::Pause24
        };
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            if (icons) {
                for (const WindIconRef& ref : kIconHeavy) {
                    (void)icons->ResolveIcon(ref);
                }
                icons->OnFrame(frame, overlay, nullptr);
            }
            SnapshotIcons(overlay);
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("icon_heavy", n, us, before, residency.Stats()));
    }

    // --- Glyph-heavy ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            if (text) {
                text->BeginFrame();
                for (int g = 0; g < 40; ++g) {
                    const std::string s = "GlyphHeavy_" + std::to_string(i) + "_" + std::to_string(g)
                        + "_日本語_αβγ";
                    (void)text->MeasureText(s, 13.0f, false);
                }
                text->OnResidencyTick();
            }
            SnapshotIcons(overlay);
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("glyph_heavy", n, us, before, residency.Stats()));
    }

    // --- Repeated open/close (labels as stand-in for panel chrome) ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            auto col = std::make_shared<Column>();
            for (int r = 0; r < 24; ++r) {
                col->AddChild(std::make_shared<Label>("Row " + std::to_string(r)));
            }
            PaintContext paint;
            paint.Clear();
            col->Measure(Size{ 400.0f, 720.0f });
            col->Arrange(Rect{ 0, 0, 400.0f, 720.0f });
            col->PaintSubtree(paint);
            col.reset();
            if (text) {
                text->OnResidencyTick();
            }
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("open_close", n, us, before, residency.Stats()));
    }

    // --- Scroll / virtualized-ish repeated measure ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        std::vector<std::shared_ptr<Label>> rows;
        auto col = std::make_shared<Column>();
        for (int r = 0; r < 80; ++r) {
            auto label = std::make_shared<Label>("ScrollItem " + std::to_string(r));
            rows.push_back(label);
            col->AddChild(label);
        }
        col->Measure(Size{ 320.0f, 600.0f });
        col->Arrange(Rect{ 0, 0, 320.0f, 600.0f });
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            rows[i % rows.size()]->SetText("ScrollItem " + std::to_string(i));
            col->Measure(Size{ 320.0f, 600.0f });
            col->Arrange(Rect{ 0, 0, 320.0f, 600.0f });
            PaintContext paint;
            paint.Clear();
            col->PaintSubtree(paint);
            if (text) {
                text->OnResidencyTick();
            }
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("scroll", n, us, before, residency.Stats()));
    }

    // --- Popup / overlay ---
    {
        residency.ResetStats();
        const auto before = residency.Stats();
        double us = 0.0;
        auto host = std::make_shared<OverlayHost>();
        host->SetBaseWidget(std::make_shared<Label>("Base"));
        const Rect vp{ 0, 0, 800.0f, 600.0f };
        host->Measure(Size{ vp.width, vp.height });
        host->Arrange(vp);
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            auto menu = std::make_shared<Column>();
            for (int m = 0; m < 8; ++m) {
                menu->AddChild(std::make_shared<Label>("Menu " + std::to_string(m)));
            }
            host->ShowPopup(menu, Point{ 40.0f, 40.0f });
            host->SyncOverlaysOnly();
            PaintContext paint;
            paint.Clear();
            host->PaintSubtree(paint);
            host->CloseAllPopups();
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("overlay", n, us, before, residency.Stats()));
    }

    // --- Resource pressure / eviction ---
    {
        residency.SetBudgets(tight);
        residency.ResetStats();
        // Load a large set once, then stop touching most icons so they become
        // over-budget / idle eviction candidates.
        if (icons) {
            static const WindIconRef kPressure[] = {
                WindIcons::Save16, WindIcons::Folder16, WindIcons::Plus16, WindIcons::Search16,
                WindIcons::Settings16, WindIcons::Box16, WindIcons::Brush16, WindIcons::Check16,
                WindIcons::Save24, WindIcons::Folder24, WindIcons::Plus24, WindIcons::Search24,
                WindIcons::Settings24, WindIcons::Box24, WindIcons::Brush24, WindIcons::Check24,
                WindIcons::Book16, WindIcons::Bug16, WindIcons::Bulb16, WindIcons::Ban16,
                WindIcons::Book24, WindIcons::Bug24, WindIcons::Bulb24, WindIcons::Ban24,
                WindIcons::Alert16, WindIcons::Body16, WindIcons::Blueprint16, WindIcons::Android16,
                WindIcons::Alert24, WindIcons::Body24, WindIcons::Blueprint24, WindIcons::Android24
            };
            for (const WindIconRef& ref : kPressure) {
                (void)icons->ResolveIcon(ref);
            }
        }
        const auto before = residency.Stats();
        double us = 0.0;
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            // Keep only one hot icon resident; leave the rest for budget eviction.
            if (icons) {
                (void)icons->ResolveIcon(WindIcons::Save16);
                icons->OnFrame(frame, overlay, nullptr);
            }
            if (text) {
                for (int g = 0; g < 20; ++g) {
                    (void)text->MeasureText("Pressure_" + std::to_string(i) + "_" + std::to_string(g), 12.0f, false);
                }
                text->OnResidencyTick();
            }
            SnapshotIcons(overlay);
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("pressure_evict", n, us, before, residency.Stats()));
        residency.SetBudgets(saved);
    }

    // --- Re-request after eviction ---
    {
        residency.SetBudgets(tight);
        residency.ResetStats();
        // Force idle + eviction of any warm icons.
        for (uint32_t warm = 0; warm < 8; ++warm) {
            AdvanceFrame(frame, overlay);
            if (icons) {
                icons->OnFrame(frame, overlay, nullptr);
            }
        }
        const auto before = residency.Stats();
        double us = 0.0;
        for (uint32_t i = 0; i < n; ++i) {
            const auto t0 = clock::now();
            AdvanceFrame(frame, overlay);
            if (icons) {
                (void)icons->ResolveIcon(WindIcons::Save16);
            }
            if (text) {
                (void)text->MeasureText("ReRequest_After_Evict", 13.0f, false);
                text->OnResidencyTick();
            }
            SnapshotIcons(overlay);
            us += Micros(t0, clock::now());
        }
        report.scenarios.push_back(Finish("rerequest_after_evict", n, us, before, residency.Stats()));
        residency.SetBudgets(saved);
    }

    (void)UIRepaintGate::ConsumeNeedsPaint();
    (void)UIRepaintGate::ConsumeNeedsLayout();

    std::ostringstream summary;
    summary << "residency scenarios=" << report.scenarios.size();
    for (const auto& s : report.scenarios) {
        summary << " | " << s.name
            << " frame=" << static_cast<uint64_t>(s.avgFrameMicros) << "us"
            << " icons=" << s.residentIcons
            << " up=" << s.uploads
            << " ev=" << s.evictions
            << " hit=" << s.cacheHits
            << " miss=" << s.cacheMisses;
    }
    report.summary = summary.str();
    return report;
}

} // namespace we::runtime::kindui
