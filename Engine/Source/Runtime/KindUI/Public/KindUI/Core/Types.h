#pragma once

#include "KindUI/Export.h"

#include <cstddef>
#include <cstdint>

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

    [[nodiscard]] bool IsEmpty() const {
        return width <= 0.0f || height <= 0.0f;
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
    // sRGB-encoded RGB in 0..1 (palette HEX authoring space). Alpha is linear.
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static Color White() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static Color Black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static Color Transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    // Discrete state pick at t >= 0.5 — no channel interpolation.
    [[nodiscard]] Color Pick(const Color& other, float t) const;
    static Color Pick(const Color& a, const Color& b, float t);
};

namespace detail {
constexpr uint8_t HexValue(char c) {
    return (c >= '0' && c <= '9') ? static_cast<uint8_t>(c - '0')
         : (c >= 'A' && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10)
         : (c >= 'a' && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10)
         : 0;
}

constexpr uint8_t HexByte(char hi, char lo) {
    return static_cast<uint8_t>((HexValue(hi) << 4) | HexValue(lo));
}

constexpr float HexChannel(uint8_t v) {
    return static_cast<float>(v) / 255.0f;
}
} // namespace detail

// Parse "#RRGGBB" or "#RRGGBBAA" — returns sRGB authoring values (UE StyleColors).
template <size_t N>
constexpr Color Hex(const char (&value)[N]) {
    const size_t start = (value[0] == '#') ? 1u : 0u;
    const uint8_t r = detail::HexByte(value[start + 0], value[start + 1]);
    const uint8_t g = detail::HexByte(value[start + 2], value[start + 3]);
    const uint8_t b = detail::HexByte(value[start + 4], value[start + 5]);
    float a = 1.0f;
    if ((N - start) >= 9) {
        a = detail::HexChannel(detail::HexByte(value[start + 6], value[start + 7]));
    }
    return {
        detail::HexChannel(r),
        detail::HexChannel(g),
        detail::HexChannel(b),
        a
    };
}

enum class HorizontalAlignment { Left, Center, Right, Fill };
enum class VerticalAlignment { Top, Center, Bottom, Fill };

} // namespace we::runtime::kindui

#include "KindUI/Core/ColorSpace.h"
