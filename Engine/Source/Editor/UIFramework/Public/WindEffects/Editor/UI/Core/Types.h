#pragma once

#include "WindEffects/Editor/UI/Export.h"

namespace WindEffects::Editor::UI {

struct UIFRAMEWORK_API Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct UIFRAMEWORK_API Size {
    float width = 0.0f;
    float height = 0.0f;
};

struct UIFRAMEWORK_API Margin {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct UIFRAMEWORK_API Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] bool Contains(Point p) const {
        return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
    }
};

struct UIFRAMEWORK_API Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    [[nodiscard]] Color Lerp(const Color& other, float t) const {
        return {
            r + (other.r - r) * t,
            g + (other.g - g) * t,
            b + (other.b - b) * t,
            a + (other.a - a) * t
        };
    }
};

enum class HorizontalAlignment { Left, Center, Right, Fill };
enum class VerticalAlignment { Top, Center, Bottom, Fill };

} // namespace WindEffects::Editor::UI
