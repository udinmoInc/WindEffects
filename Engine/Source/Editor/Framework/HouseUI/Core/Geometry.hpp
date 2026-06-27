#pragma once

namespace HouseEngine::UI {

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

    bool Contains(const Point& p) const {
        return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
    }

    Rect Intersect(const Rect& other) const {
        float nx = (x > other.x) ? x : other.x;
        float ny = (y > other.y) ? y : other.y;
        float r1 = x + width;
        float r2 = other.x + other.width;
        float nr = (r1 < r2) ? r1 : r2;
        float b1 = y + height;
        float b2 = other.y + other.height;
        float nb = (b1 < b2) ? b1 : b2;

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

    static Color White() { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
    static Color Black() { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
    static Color Transparent() { return { 0.0f, 0.0f, 0.0f, 0.0f }; }

    // Color operations
    Color operator*(float scalar) const {
        return { r * scalar, g * scalar, b * scalar, a * scalar };
    }

    static Color Lerp(const Color& a, const Color& b, float t) {
        return {
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        };
    }
};

} // namespace HouseEngine::UI
