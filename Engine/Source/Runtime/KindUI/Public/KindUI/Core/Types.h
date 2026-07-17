#pragma once

#include "KindUI/Export.h"

namespace we::runtime::kindui {

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct Size {
    float width = 0.0f;
    float height = 0.0f;
};

struct Margin {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    [[nodiscard]] bool Contains(Point p) const {
        return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
    }

    [[nodiscard]] Rect Intersect(const Rect& other) const {
        const float nx = (x > other.x) ? x : other.x;
        const float ny = (y > other.y) ? y : other.y;
        const float r1 = x + width;
        const float r2 = other.x + other.width;
        const float nr = (r1 < r2) ? r1 : r2;
        const float b1 = y + height;
        const float b2 = other.y + other.height;
        const float nb = (b1 < b2) ? b1 : b2;

        float nw = nr - nx;
        float nh = nb - ny;
        if (nw < 0.0f) nw = 0.0f;
        if (nh < 0.0f) nh = 0.0f;
        return { nx, ny, nw, nh };
    }
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    Color operator*(float scalar) const {
        return { r * scalar, g * scalar, b * scalar, a * scalar };
    }

    [[nodiscard]] Color Lerp(const Color& other, float t) const {
        return {
            r + (other.r - r) * t,
            g + (other.g - g) * t,
            b + (other.b - b) * t,
            a + (other.a - a) * t
        };
    }

    static Color Lerp(const Color& a, const Color& b, float t) {
        return a.Lerp(b, t);
    }
};

enum class HorizontalAlignment { Left, Center, Right, Fill };
enum class VerticalAlignment { Top, Center, Bottom, Fill };

} // namespace we::runtime::kindui
