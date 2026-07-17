#include "KindUI/Core/UIRepaintGate.h"

namespace we::runtime::kindui {

bool UIRepaintGate::s_NeedsRebuild = true;
bool UIRepaintGate::s_Animating = false;
uint64_t UIRepaintGate::s_RebuildCount = 0;
uint64_t UIRepaintGate::s_SkipCount = 0;

void UIRepaintGate::Request() {
    s_NeedsRebuild = true;
}

void UIRepaintGate::MarkAnimating() {
    s_Animating = true;
    s_NeedsRebuild = true;
}

void UIRepaintGate::MarkSettled() {
    s_Animating = false;
}

bool UIRepaintGate::ConsumeNeedsRebuild() {
    // Animating is a one-frame latch: Tick/Damp must re-assert MarkAnimating each frame
    // while still moving, otherwise idle frames keep rebuilding forever.
    const bool needs = s_NeedsRebuild || s_Animating;
    if (needs) {
        ++s_RebuildCount;
        s_NeedsRebuild = false;
        s_Animating = false;
        return true;
    }
    ++s_SkipCount;
    return false;
}

bool UIRepaintGate::PeekNeedsRebuild() {
    return s_NeedsRebuild || s_Animating;
}

uint64_t UIRepaintGate::RebuildCount() {
    return s_RebuildCount;
}

uint64_t UIRepaintGate::SkipCount() {
    return s_SkipCount;
}

} // namespace we::runtime::kindui
