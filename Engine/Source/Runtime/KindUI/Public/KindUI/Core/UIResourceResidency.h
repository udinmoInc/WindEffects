// ==============================================================================
// WindEffects — KindUI — UIResourceResidency
// Global UI resource residency budgets, frame clock, and statistics.
// IconManager / TextUIService / UiImmediateRenderer remain owners — this
// coordinator supplies budgets, pin horizon, and aggregate counters so future
// panels inherit residency policy without per-widget caches.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

enum class UIResourceState : uint8_t {
    Unloaded = 0,
    Loading = 1,
    Resident = 2,
    Evictable = 3,
};

enum class UIResourceKind : uint8_t {
    IconTexture = 0,
    TextGeometry = 1,
    FontAtlasGlyph = 2,
};

/// Configurable budgets (env overrides; defaults keep normal editor interaction warm).
struct KINDUI_API UIResidencyBudgets {
    uint64_t iconGpuBudgetBytes = 48ull * 1024ull * 1024ull;       ///< ~48 MiB icon GPU
    uint64_t textGeometryBudgetBytes = 8ull * 1024ull * 1024ull;   ///< ~8 MiB CPU glyph quads
    uint32_t glyphEntryBudget = 8192;                              ///< atlas glyph entries
    uint32_t iconIdleFramesBeforeEvictable = 1800;                 ///< ~30s @ 60fps
    uint32_t textGeomIdleFramesBeforeEvictable = 900;
    uint32_t maxEvictionsPerTick = 8;
    uint32_t tickIntervalFrames = 30;                              ///< avoid full scans every frame
    uint32_t gpuDeferFrames = 3;                                   ///< FIF+1 style horizon
};

struct KINDUI_API UIResidencyStats {
    uint32_t residentIconCount = 0;
    uint32_t residentTextGeomCount = 0;
    uint32_t residentGlyphCount = 0;
    uint64_t residentIconGpuBytes = 0;
    uint64_t residentTextGeomCpuBytes = 0;
    uint64_t residentAtlasCpuBytes = 0;
    uint64_t loads = 0;
    uint64_t uploads = 0;
    uint64_t evictions = 0;
    uint64_t cacheHits = 0;
    uint64_t cacheMisses = 0;
    uint64_t deferredReleases = 0;
    uint64_t evictionPasses = 0;
    uint64_t skippedEvictionPasses = 0;
};

/// Process-wide residency policy for KindUI GPU/CPU UI resources.
class KINDUI_API UIResourceResidency {
public:
    static UIResourceResidency& Get();

    void LoadBudgetsFromEnv();
    [[nodiscard]] const UIResidencyBudgets& Budgets() const { return m_Budgets; }
    void SetBudgets(const UIResidencyBudgets& budgets) { m_Budgets = budgets; }

    /// Call once per UI frame (OverlayRenderer::RenderUI).
    void BeginFrame(uint64_t frameNumber, uint32_t framesInFlight);

    [[nodiscard]] uint64_t CurrentFrame() const { return m_Frame; }
    [[nodiscard]] uint32_t GpuDeferFrames() const { return m_Budgets.gpuDeferFrames; }
    /// Frames until deferred GPU destroys may run after PrepareForResourceEviction.
    [[nodiscard]] uint64_t PinHorizon() const { return m_PinHorizon; }
    void PinThroughGpuHorizon();

    [[nodiscard]] bool ShouldRunEvictionPass() const;
    void NoteEvictionPass(bool performed);
    [[nodiscard]] uint32_t MaxEvictionsThisTick() const { return m_Budgets.maxEvictionsPerTick; }

    [[nodiscard]] uint64_t IconGpuBudgetBytes() const { return m_Budgets.iconGpuBudgetBytes; }
    [[nodiscard]] uint64_t TextGeometryBudgetBytes() const { return m_Budgets.textGeometryBudgetBytes; }
    [[nodiscard]] uint32_t GlyphEntryBudget() const { return m_Budgets.glyphEntryBudget; }
    [[nodiscard]] uint32_t IconIdleFrames() const { return m_Budgets.iconIdleFramesBeforeEvictable; }
    [[nodiscard]] uint32_t TextGeomIdleFrames() const { return m_Budgets.textGeomIdleFramesBeforeEvictable; }

    void NoteHit();
    void NoteMiss();
    void NoteLoad();
    void NoteUpload();
    void NoteEviction(uint32_t count = 1);
    void NoteDeferredRelease(uint32_t count = 1);

    void SetResidentSnapshot(
        uint32_t iconCount,
        uint64_t iconGpuBytes,
        uint32_t textGeomCount,
        uint64_t textGeomBytes,
        uint32_t glyphCount,
        uint64_t atlasCpuBytes);

    [[nodiscard]] const UIResidencyStats& Stats() const { return m_Stats; }
    void ResetStats();

private:
    UIResourceResidency() = default;

    UIResidencyBudgets m_Budgets{};
    UIResidencyStats m_Stats{};
    uint64_t m_Frame = 0;
    uint64_t m_LastEvictionPassFrame = 0;
    uint64_t m_PinHorizon = 0;
    bool m_BudgetsLoaded = false;
    bool m_EvictionTickThisFrame = false;
    bool m_EvictionPassNotedThisFrame = false;
};

} // namespace we::runtime::kindui
