#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include <cstdint>

namespace WindEffects::Editor::UI {

// Dirty-state gate for editor chrome. Request on input/size/data changes;
// MarkAnimating while hover/press damps so idle frames can skip UI rebuild.
class UI_API UIRepaintGate {
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
    static bool s_NeedsRebuild;
    static bool s_Animating;
    static uint64_t s_RebuildCount;
    static uint64_t s_SkipCount;
};

} // namespace WindEffects::Editor::UI
