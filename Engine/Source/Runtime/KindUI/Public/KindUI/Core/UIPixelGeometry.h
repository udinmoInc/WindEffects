// ==============================================================================
// WindEffects — KindUI — UIPixelGeometry
// Public API surface for the KindUI module.
// Reusable internal UI pixel-snapping and DPI-aware geometry policy.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Types.h"

#include <cmath>
#include <algorithm>

namespace we::runtime::kindui {

class KINDUI_API UIPixelGeometry {
public:
    /// Snap a float pixel coordinate to the nearest integer.
    static float SnapPx(float v) {
        return std::floor(v + 0.5f);
    }

    /// Snap a logical value scaled by active DPI.
    static float ScaleSnap(float logicalPx) {
        return DPIContext::ScaleSnap(logicalPx);
    }

    /// Snap a rectangle bounds.
    static Rect SnapRect(const Rect& rect) {
        const float x0 = SnapPx(rect.x);
        const float y0 = SnapPx(rect.y);
        const float x1 = SnapPx(rect.x + rect.width);
        const float y1 = SnapPx(rect.y + rect.height);
        return Rect{ x0, y0, x1 - x0, y1 - y0 };
    }

    /// Snap line endpoints and thickness for crisp physical 1px alignment.
    static void SnapLine(Point start, Point end, float thickness, Point& outStart, Point& outEnd, float& outThickness) {
        outStart = Point{ SnapPx(start.x), SnapPx(start.y) };
        outEnd = Point{ SnapPx(end.x), SnapPx(end.y) };
        outThickness = std::max(1.0f, SnapPx(thickness));
    }

    /// Evaluate DPI scale consistency across subsystems.
    static const char* EvaluateDpiConsistency(float contextScale, float themeScale) {
        const bool match = std::abs(contextScale - themeScale) < 0.001f && contextScale >= 1.0f;
        return match ? "PASS" : "FAIL";
    }
};

} // namespace we::runtime::kindui
