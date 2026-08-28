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

    static void MarkAnimating();
    static void MarkSettled();
    /// Clears per-frame animation latch; call once at the start of each UI frame.
    static void BeginFrame();

    // Returns true if Measure/Arrange should run this frame.
    [[nodiscard]] static bool ConsumeNeedsLayout();
    // Returns true if Paint + geometry upload should run this frame.
    [[nodiscard]] static bool ConsumeNeedsPaint();

    // Legacy: true if either layout or paint is dirty.
    [[nodiscard]] static bool ConsumeNeedsRebuild();
    [[nodiscard]] static bool PeekNeedsRebuild();
    [[nodiscard]] static bool PeekNeedsLayout();
    [[nodiscard]] static bool PeekNeedsPaint();

    [[nodiscard]] static uint64_t RebuildCount();
    [[nodiscard]] static uint64_t SkipCount();
    [[nodiscard]] static uint64_t LayoutRebuildCount();
    [[nodiscard]] static uint64_t PaintRebuildCount();
    [[nodiscard]] static uint64_t IdleSkipCount();

private:
    static std::atomic<bool> s_NeedsLayout;
    static std::atomic<bool> s_NeedsPaint;
    static std::atomic<bool> s_Animating;
    static std::atomic<uint64_t> s_RebuildCount;
    static std::atomic<uint64_t> s_SkipCount;
    static std::atomic<uint64_t> s_LayoutRebuildCount;
    static std::atomic<uint64_t> s_PaintRebuildCount;
    static std::atomic<uint64_t> s_IdleSkipCount;
};

} // namespace we::runtime::kindui
