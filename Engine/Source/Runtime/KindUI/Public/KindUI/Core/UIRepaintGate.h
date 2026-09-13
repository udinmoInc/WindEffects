// ==============================================================================
// WindEffects — KindUI — UIRepaintGate
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <atomic>
#include <cstdint>

namespace we::runtime::kindui {

/// Dirty-state gate for editor chrome. Request on input/size/data changes;
/// MarkAnimating while hover/press damps so idle frames can skip UI rebuild.
///
/// Layout and paint invalidation are tracked separately so hover/paint-only
/// changes do not force a full Measure/Arrange pass on the widget tree.
///
/// Thread model: all members are atomics. Request/Mark* may be called from any
/// thread that posts UI work; Consume* is intended for the UI/render thread.
class KINDUI_API UIRepaintGate {
public:
    /// Marks both layout and paint dirty (backward-compatible full invalidation).
    static void Request();
    static void RequestLayout();
    static void RequestPaint();

    /// Same as RequestLayout/RequestPaint but tags a stable reason for WE_UI_INVALIDATION_LOG=1.
    static void RequestLayoutReason(const char* reason);
    static void RequestPaintReason(const char* reason);
    [[nodiscard]] static const char* LastLayoutReason();
    [[nodiscard]] static const char* LastPaintReason();
    [[nodiscard]] static uint64_t LayoutReasonCount();
    [[nodiscard]] static uint64_t PaintReasonCount();

    static void MarkAnimating();
    /// Clears the sticky animation latch. Call at the start of the widget-tick phase
    /// after PeekNeedsWidgetTick() so Tick must re-arm via MarkAnimating/Damp.
    static void MarkSettled();
    /// Frame bookkeeping. Does not clear the animation latch — that latch must survive
    /// ConsumeNeedsPaint so the next frame still runs widget Tick while damping.
    static void BeginFrame();

    /// Scoped batching for multi-step structural operations (float/dock a panel,
    /// swap a dock, teardown/recreate floating hosts). While a batch is open the
    /// Request*/Mark* calls are recorded as deferred and collapsed into a single
    /// flag set (plus one cause-log entry per deferred category) when the outermost
    /// batch closes, so a multi-step operation produces exactly one layout pass and
    /// one paint pass instead of one per mutated widget.
    static void BeginBatch();
    static void EndBatch();
    [[nodiscard]] static bool InBatch();

    /// RAII wrapper; use as a stack guard around a structural operation.
    struct ScopedBatch {
        ScopedBatch() { UIRepaintGate::BeginBatch(); }
        ~ScopedBatch() { UIRepaintGate::EndBatch(); }
        ScopedBatch(const ScopedBatch&) = delete;
        ScopedBatch& operator=(const ScopedBatch&) = delete;
    };

    // Returns true if Measure/Arrange should run this frame.
    [[nodiscard]] static bool ConsumeNeedsLayout();
    // Returns true if Paint + geometry upload should run this frame.
    [[nodiscard]] static bool ConsumeNeedsPaint();

    // Legacy: true if either layout or paint is dirty.
    [[nodiscard]] static bool ConsumeNeedsRebuild();
    [[nodiscard]] static bool PeekNeedsRebuild();
    [[nodiscard]] static bool PeekNeedsLayout();
    [[nodiscard]] static bool PeekNeedsPaint();

    /// True when layout and paint are both clean (no animation latch).
    [[nodiscard]] static bool PeekIsFullyIdle();
    /// True when widget Tick / hover-damp should run this frame.
    [[nodiscard]] static bool PeekNeedsWidgetTick();

    [[nodiscard]] static uint64_t RebuildCount();
    [[nodiscard]] static uint64_t SkipCount();
    [[nodiscard]] static uint64_t LayoutRebuildCount();
    [[nodiscard]] static uint64_t PaintRebuildCount();
    [[nodiscard]] static uint64_t IdleSkipCount();

private:
    static std::atomic<bool> s_NeedsLayout;
    static std::atomic<bool> s_NeedsPaint;
    static std::atomic<bool> s_Animating;
    static std::atomic<int> s_BatchDepth;
    static std::atomic<bool> s_BatchDeferredLayout;
    static std::atomic<bool> s_BatchDeferredPaint;
    static std::atomic<bool> s_BatchDeferredAnimating;
    static std::atomic<uint64_t> s_RebuildCount;
    static std::atomic<uint64_t> s_SkipCount;
    static std::atomic<uint64_t> s_LayoutRebuildCount;
    static std::atomic<uint64_t> s_PaintRebuildCount;
    static std::atomic<uint64_t> s_IdleSkipCount;
    static std::atomic<const char*> s_LastLayoutReason;
    static std::atomic<const char*> s_LastPaintReason;
    static std::atomic<uint64_t> s_LayoutReasonCount;
    static std::atomic<uint64_t> s_PaintReasonCount;
};

} // namespace we::runtime::kindui
