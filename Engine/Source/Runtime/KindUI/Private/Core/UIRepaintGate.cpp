#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Profiling/PaintCauseLog.h"

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
        s_NeedsLayout.store(true, std::memory_order_release);
    }
    if (deferPaint || deferAnimating) {
        PaintCauseLog::Get().Push("gate-paint", WE_PAINT_CALLER);
        s_NeedsPaint.store(true, std::memory_order_release);
    }
}

bool UIRepaintGate::InBatch() {
    return s_BatchDepth.load(std::memory_order_acquire) > 0;
}

void UIRepaintGate::Request() {
    if (InBatch()) {
        s_BatchDeferredLayout.store(true, std::memory_order_relaxed);
        s_BatchDeferredPaint.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-all", WE_PAINT_CALLER);
    s_NeedsLayout.store(true, std::memory_order_release);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestLayout() {
    if (InBatch()) {
        s_BatchDeferredLayout.store(true, std::memory_order_relaxed);
        return;
    }
    PaintCauseLog::Get().Push("gate-layout", WE_PAINT_CALLER);
    s_NeedsLayout.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestPaint() {
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
    s_Animating.store(true, std::memory_order_release);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::BeginFrame() {
    s_Animating.store(false, std::memory_order_release);
}

void UIRepaintGate::MarkSettled() {
    s_Animating.store(false, std::memory_order_release);
}

bool UIRepaintGate::ConsumeNeedsLayout() {
    const bool animating = s_Animating.load(std::memory_order_acquire);
    const bool requested = s_NeedsLayout.exchange(false, std::memory_order_acq_rel);
    const bool needs = requested;
    if (needs) {
        s_LayoutRebuildCount.fetch_add(1, std::memory_order_relaxed);
    }
    return needs;
}

bool UIRepaintGate::ConsumeNeedsPaint() {
    const bool requested = s_NeedsPaint.exchange(false, std::memory_order_acq_rel);
    const bool needs = requested;
    if (needs) {
        s_PaintRebuildCount.fetch_add(1, std::memory_order_relaxed);
        s_RebuildCount.fetch_add(1, std::memory_order_relaxed);
    } else {
        s_IdleSkipCount.fetch_add(1, std::memory_order_relaxed);
        s_SkipCount.fetch_add(1, std::memory_order_relaxed);
    }
    return needs;
}

bool UIRepaintGate::ConsumeNeedsRebuild() {
    (void)ConsumeNeedsLayout();
    return ConsumeNeedsPaint();
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

} // namespace we::runtime::kindui
