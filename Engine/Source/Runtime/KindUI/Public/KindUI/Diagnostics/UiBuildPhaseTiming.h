#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

/// Per-rebuild CPU phase timings for ProcessWidget (milliseconds).
/// Populated when WE_UI_BUILD_PROFILE=1 or always (cheap; logged only when enabled).
struct KINDUI_API UiBuildPhaseTiming {
    float clearMs = 0.0f;
    float layoutMs = 0.0f;   // Measure+Arrange inside adapter (usually 0; host owns layout)
    float paintMs = 0.0f;    // Widget::Paint + DrawCommand recording
    float drawgenMs = 0.0f;  // ConvertDrawCommand (rects/icons/…)
    float textMs = 0.0f;     // Text path inside drawgen
    float clearDirtyMs = 0.0f;
    float totalMs = 0.0f;
    uint32_t paintCommands = 0;
    uint32_t textCommands = 0;
    uint32_t rectCommands = 0;
    uint32_t vertices = 0;
    uint32_t batches = 0;
    uint32_t subtreesPainted = 0;
    uint32_t subtreesReplayed = 0;
    uint32_t commandsReplayed = 0;
    bool ranLayout = false;
    bool paintRetention = false;
};

} // namespace we::runtime::kindui
