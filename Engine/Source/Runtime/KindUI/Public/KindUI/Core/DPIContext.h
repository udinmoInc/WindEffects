#pragma once

#include "KindUI/Export.h"

#include <cmath>

namespace we::runtime::kindui {

class DPIContext {
public:
    KINDUI_API static float GetScale();
    KINDUI_API static void SetScale(float scale);

    static float Scale(float value) { return value * GetScale(); }

    /// Snap a device-pixel value to the nearest integer (crisp borders / text baselines).
    static float Snap(float devicePx) { return std::round(devicePx); }

    /// Scale logical px by DPI, then snap to device pixels.
    static float ScaleSnap(float logicalPx) { return Snap(Scale(logicalPx)); }
};

} // namespace we::runtime::kindui
