// ==============================================================================
// WindEffects — KindUI — UIRepaintGate
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Profiling/PaintCauseLog.h"

#include "Core/Logger.h"

#include <cstdlib>
#include <string>

namespace we::runtime::kindui {

std::atomic<bool> UIRepaintGate::s_NeedsLayout{true};
std::atomic<bool> UIRepaintGate::s_NeedsPaint{true};
std::atomic<bool> UIRepaintGate::s_Animating{false};
std::atomic<int> UIRepaintGate::s_BatchDepth{0};
std::atomic<bool> UIRepaintGate::s_BatchDeferredLayout{false};
std::atomic<bool> UIRepaintGate::s_BatchDeferredPaint{false};
std::atomic<bool> UIRepaintGate::s_BatchDeferredAnimating{false};
std::atomic<uint64_t> UIRepaintGate::s_RebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_SkipCount{0};
std::atomic<uint64_t> UIRepaintGate::s_LayoutRebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_PaintRebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_IdleSkipCount{0};
std::atomic<const char*> UIRepaintGate::s_LastLayoutReason{nullptr};
std::atomic<const char*> UIRepaintGate::s_LastPaintReason{nullptr};
std::atomic<uint64_t> UIRepaintGate::s_LayoutReasonCount{0};
std::atomic<uint64_t> UIRepaintGate::s_PaintReasonCount{0};

namespace {

[[nodiscard]] bool InvalidationLogEnabled() {
    const char* v = std::getenv("WE_UI_INVALIDATION_LOG");
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

[[nodiscard]] const char* NormalizeReason(const char* reason) {
    return (reason && reason[0]) ? reason : "Unknown";
}

} // namespace

void UIRepaintGate::BeginBatch() {
    s_BatchDepth.fetch_add(1, std::memory_order_relaxed);
}

void UIRepaintGate::EndBatch() {
    const int remaining = s_BatchDepth.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (remaining != 0) {
        return;
    }
    // Outermost batch closed: collapse every deferred request into one flag set.
    const bool deferLayout = s_BatchDeferredLayout.exchange(false, std::memory_order_acq_rel);
    const bool deferPaint = s_BatchDeferredPaint.exchange(false, std::memory_order_acq_rel);
    const bool deferAnimating = s_BatchDeferredAnimating.exchange(false, std::memory_order_acq_rel);
    if (deferAnimating) {
        s_Animating.store(true, std::memory_order_release);
    }
    if (deferLayout) {
        PaintCauseLog::Get().Push("gate-layout", WE_PAINT_CALLER);
        s_LastLayoutReason.store("Batch", std::memory_order_relaxed);
        s_LayoutReasonCount.fetch_add(1, std::memory_order_relaxed);
        s_NeedsLayout.store(true, std::memory_order_release);
    }
    if (deferPaint || deferAnimating) {
        PaintCauseLog::Get().Push("gate-paint", WE_PAINT_CALLER);
        s_LastPaintReason.store(deferAnimating ? "Animation" : "Batch", std::memory_order_relaxed);
        s_PaintReasonCount.fetch_add(1, std::memory_order_relaxed);
        s_NeedsPaint.store(true, std::memory_order_release);
    }
}

bool UIRepaintGate::InBatch() {
    // Relaxed is sufficient here: the actual dirty flag stores in RequestLayout/RequestPaint/MarkAnimating
    // use memory_order_release, which provides the necessary ordering guarantee to observers.
    return s_BatchDepth.load(std::memory_order_relaxed) > 0;
}

void UIRepaintGate::Request() {
    if (InBatch()) {
        s_BatchDeferredLayout.store(true, std::memory_order_relaxed);
        s_BatchDeferredPaint.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-all", WE_PAINT_CALLER);
    s_LastLayoutReason.store("Unknown", std::memory_order_relaxed);
    s_LastPaintReason.store("Unknown", std::memory_order_relaxed);
    s_LayoutReasonCount.fetch_add(1, std::memory_order_relaxed);
    s_PaintReasonCount.fetch_add(1, std::memory_order_relaxed);
    s_NeedsLayout.store(true, std::memory_order_release);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestLayout() {
    RequestLayoutReason("Unknown");
}

void UIRepaintGate::RequestPaint() {
    RequestPaintReason("Unknown");
}

void UIRepaintGate::RequestLayoutReason(const char* reason) {
    reason = NormalizeReason(reason);
    s_LastLayoutReason.store(reason, std::memory_order_relaxed);
    s_LayoutReasonCount.fetch_add(1, std::memory_order_relaxed);
    if (InvalidationLogEnabled()) {
        HE_INFO(std::string("[UIInvalidation] layout reason=") + reason);
    }
    if (InBatch()) {
        s_BatchDeferredLayout.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-layout", WE_PAINT_CALLER);
    s_NeedsLayout.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestPaintReason(const char* reason) {
    reason = NormalizeReason(reason);
    s_LastPaintReason.store(reason, std::memory_order_relaxed);
    s_PaintReasonCount.fetch_add(1, std::memory_order_relaxed);
    if (InvalidationLogEnabled()) {
        HE_INFO(std::string("[UIInvalidation] paint reason=") + reason);
    }
    if (InBatch()) {
        s_BatchDeferredPaint.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-paint", WE_PAINT_CALLER);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::MarkAnimating() {
    if (InBatch()) {
        s_BatchDeferredAnimating.store(true, std::memory_order_relaxed);
        s_BatchDeferredPaint.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-anim", WE_PAINT_CALLER);
    s_LastPaintReason.store("Animation", std::memory_order_relaxed);
    s_PaintReasonCount.fetch_add(1, std::memory_order_relaxed);
    s_Animating.store(true, std::memory_order_release);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::BeginFrame() {
    // Intentionally leave s_Animating set. ConsumeNeedsPaint clears paint dirty each
    // frame; the animating latch is what keeps PeekNeedsWidgetTick true across frames
    // until the tick phase MarkSettled() + Tick re-arms (or settles idle).
}

void UIRepaintGate::MarkSettled() {
    s_Animating.store(false, std::memory_order_release);
}

bool UIRepaintGate::ConsumeNeedsLayout() {
    const bool requested = s_NeedsLayout.exchange(false, std::memory_order_acq_rel);
    if (requested) {
        s_LayoutRebuildCount.fetch_add(1, std::memory_order_relaxed);
    }
    return requested;
}

bool UIRepaintGate::ConsumeNeedsPaint() {
    const bool requested = s_NeedsPaint.exchange(false, std::memory_order_acq_rel);
    if (requested) {
        s_PaintRebuildCount.fetch_add(1, std::memory_order_relaxed);
        s_RebuildCount.fetch_add(1, std::memory_order_relaxed);
    } else {
        s_IdleSkipCount.fetch_add(1, std::memory_order_relaxed);
        s_SkipCount.fetch_add(1, std::memory_order_relaxed);
    }
    return requested;
}

bool UIRepaintGate::ConsumeNeedsRebuild() {
    // Consume both flags and return true if either needs a rebuild.
    const bool needsLayout = ConsumeNeedsLayout();
    const bool needsPaint = ConsumeNeedsPaint();
    return needsLayout || needsPaint;
}

bool UIRepaintGate::PeekNeedsLayout() {
    return s_NeedsLayout.load(std::memory_order_acquire);
}

bool UIRepaintGate::PeekNeedsPaint() {
    return s_NeedsPaint.load(std::memory_order_acquire)
        || s_Animating.load(std::memory_order_acquire);
}

bool UIRepaintGate::PeekNeedsRebuild() {
    return PeekNeedsLayout() || PeekNeedsPaint();
}

bool UIRepaintGate::PeekIsFullyIdle() {
    return !PeekNeedsRebuild();
}

bool UIRepaintGate::PeekNeedsWidgetTick() {
    // Hover/press damp, caret-adjacent focus anim, and structural layout all need Tick.
    // When fully idle, skip the widget-tree walk entirely.
    return PeekNeedsRebuild();
}

uint64_t UIRepaintGate::RebuildCount() {
    return s_RebuildCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::SkipCount() {
    return s_SkipCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::LayoutRebuildCount() {
    return s_LayoutRebuildCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::PaintRebuildCount() {
    return s_PaintRebuildCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::IdleSkipCount() {
    return s_IdleSkipCount.load(std::memory_order_relaxed);
}

const char* UIRepaintGate::LastLayoutReason() {
    const char* reason = s_LastLayoutReason.load(std::memory_order_relaxed);
    return reason ? reason : "None";
}

const char* UIRepaintGate::LastPaintReason() {
    const char* reason = s_LastPaintReason.load(std::memory_order_relaxed);
    return reason ? reason : "None";
}

uint64_t UIRepaintGate::LayoutReasonCount() {
    return s_LayoutReasonCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::PaintReasonCount() {
    return s_PaintReasonCount.load(std::memory_order_relaxed);
}

} // namespace we::runtime::kindui
