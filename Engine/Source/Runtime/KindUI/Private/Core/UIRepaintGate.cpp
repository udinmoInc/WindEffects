#include "KindUI/Core/UIRepaintGate.h"

namespace we::runtime::kindui {

std::atomic<bool> UIRepaintGate::s_NeedsRebuild{true};
std::atomic<bool> UIRepaintGate::s_Animating{false};
std::atomic<uint64_t> UIRepaintGate::s_RebuildCount{0};
std::atomic<uint64_t> UIRepaintGate::s_SkipCount{0};

void UIRepaintGate::Request() {
    s_NeedsRebuild.store(true, std::memory_order_release);
}

void UIRepaintGate::MarkAnimating() {
    s_Animating.store(true, std::memory_order_release);
    s_NeedsRebuild.store(true, std::memory_order_release);
}

void UIRepaintGate::MarkSettled() {
    s_Animating.store(false, std::memory_order_release);
}

bool UIRepaintGate::ConsumeNeedsRebuild() {
    // Animating is a one-frame latch: Tick/Damp must re-assert MarkAnimating each frame
    // while still moving, otherwise idle frames keep rebuilding forever.
    const bool animating = s_Animating.exchange(false, std::memory_order_acq_rel);
    const bool requested = s_NeedsRebuild.exchange(false, std::memory_order_acq_rel);
    const bool needs = requested || animating;
    if (needs) {
        s_RebuildCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    s_SkipCount.fetch_add(1, std::memory_order_relaxed);
    return false;
}

bool UIRepaintGate::PeekNeedsRebuild() {
    return s_NeedsRebuild.load(std::memory_order_acquire)
        || s_Animating.load(std::memory_order_acquire);
}

uint64_t UIRepaintGate::RebuildCount() {
    return s_RebuildCount.load(std::memory_order_relaxed);
}

uint64_t UIRepaintGate::SkipCount() {
    return s_SkipCount.load(std::memory_order_relaxed);
}

} // namespace we::runtime::kindui
