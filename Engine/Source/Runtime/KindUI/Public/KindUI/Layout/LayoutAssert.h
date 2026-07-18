#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Geometry.h"

#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace we::runtime::kindui {

/// Debug checks for dock/workspace layout integrity.
inline void AssertLayoutRectValid(std::string_view label, const Rect& rect, const Rect& parentBounds) {
#if defined(WE_DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    const bool negative = rect.width < -0.5f || rect.height < -0.5f;
    const bool overflow =
        rect.x < parentBounds.x - 0.5f
        || rect.y < parentBounds.y - 0.5f
        || (rect.x + rect.width) > (parentBounds.x + parentBounds.width) + 0.5f
        || (rect.y + rect.height) > (parentBounds.y + parentBounds.height) + 0.5f;

    if (negative || overflow) {
        HE_ERROR(
            std::string("[Layout] Invalid rect '") + std::string(label)
            + "' size=(" + std::to_string(rect.width) + "x" + std::to_string(rect.height)
            + ") pos=(" + std::to_string(rect.x) + "," + std::to_string(rect.y)
            + ") parent=(" + std::to_string(parentBounds.width) + "x"
            + std::to_string(parentBounds.height) + ")");
    }
#else
    (void)label;
    (void)rect;
    (void)parentBounds;
#endif
}

inline void AssertNonNegativeSize(std::string_view label, float width, float height) {
#if defined(WE_DEBUG) || defined(_DEBUG) || !defined(NDEBUG)
    if (width < -0.5f || height < -0.5f) {
        HE_ERROR(
            std::string("[Layout] Negative size '") + std::string(label)
            + "' (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    }
#else
    (void)label;
    (void)width;
    (void)height;
#endif
}

inline Rect ClampRectToParent(const Rect& child, const Rect& parent) {
    Rect out = child;
    const float parentRight = parent.x + parent.width;
    const float parentBottom = parent.y + parent.height;

    out.x = std::clamp(out.x, parent.x, parentRight);
    out.y = std::clamp(out.y, parent.y, parentBottom);
    out.width = std::max(0.0f, std::min(out.width, parentRight - out.x));
    out.height = std::max(0.0f, std::min(out.height, parentBottom - out.y));
    return out;
}

} // namespace we::runtime::kindui
