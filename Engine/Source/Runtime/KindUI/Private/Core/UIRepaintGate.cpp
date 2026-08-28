#include "KindUI/Core/UIRepaintGate.h"

namespace we::runtime::kindui {

std::atomic<bool> UIRepaintGate::s_NeedsLayout{true};
std::atomic<bool> UIRepaintGate::s_NeedsPaint{true};
std::atomic<bool> UIRepaintGate::s_Animating{false};
std::atomic<uint64_t> UIRepaintGate::s_RebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_SkipCount{0};
std::atomic<uint64_t> UIRepaintGate::s_LayoutRebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_PaintRebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_IdleSkipCount{0};

void UIRepaintGate::Request() {
    s_NeedsLayout.store(true, std::memory_order_release);
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestLayout() {
    s_NeedsLayout.store(true, std::memory_order_release);
}

void UIRepaintGate::RequestPaint() {
    s_NeedsPaint.store(true, std::memory_order_release);
}

void UIRepaintGate::MarkAnimating() {
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
