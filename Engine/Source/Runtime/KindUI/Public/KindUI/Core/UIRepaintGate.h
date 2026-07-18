#pragma once

#include "KindUI/Export.h"

#include <atomic>
#include <cstdint>

namespace we::runtime::kindui {

/// Dirty-state gate for editor chrome. Request on input/size/data changes;
/// MarkAnimating while hover/press damps so idle frames can skip UI rebuild.
///
/// Thread model: all members are atomics. Request/Mark* may be called from any
/// thread that posts UI work; ConsumeNeedsRebuild is intended for the UI/render
/// thread that owns layout. Counters are approximate under contention (relaxed).
class KINDUI_API UIRepaintGate {
public:
    static void Request();
    static void MarkAnimating();
    static void MarkSettled();

    // Returns true if Measure/Arrange/Paint + geometry upload should run.
    [[nodiscard]] static bool ConsumeNeedsRebuild();

    [[nodiscard]] static bool PeekNeedsRebuild();
    [[nodiscard]] static uint64_t RebuildCount();
    [[nodiscard]] static uint64_t SkipCount();

private:
    static std::atomic<bool> s_NeedsRebuild;
    static std::atomic<bool> s_Animating;
    static std::atomic<uint64_t> s_RebuildCount;
    static std::atomic<uint64_t> s_SkipCount;
};

} // namespace we::runtime::kindui
