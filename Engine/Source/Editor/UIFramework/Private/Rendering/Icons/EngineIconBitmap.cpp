#include "Rendering/Icons/EngineIconBitmap.h"

#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>
#include <cmath>

namespace WindEffects::Editor::UI {
namespace {

std::array<uint8_t, 3> ToRgb(const Color& color) {
    return {
        static_cast<uint8_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f),
        static_cast<uint8_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f),
    };
}

std::array<uint8_t, 3> LerpRgb(const std::array<uint8_t, 3>& a, const std::array<uint8_t, 3>& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<uint8_t>(a[0] + (b[0] - a[0]) * t),
        static_cast<uint8_t>(a[1] + (b[1] - a[1]) * t),
        static_cast<uint8_t>(a[2] + (b[2] - a[2]) * t),
    };
}

std::array<uint8_t, 3> AdjustBrightness(const std::array<uint8_t, 3>& rgb, float delta) {
    return {
        static_cast<uint8_t>(std::clamp(static_cast<int>(rgb[0]) + static_cast<int>(delta * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(rgb[1]) + static_cast<int>(delta * 255.0f), 0, 255)),
        static_cast<uint8_t>(std::clamp(static_cast<int>(rgb[2]) + static_cast<int>(delta * 255.0f), 0, 255)),
    };
}

} // namespace

IconShadeRgb BuildIconShades(float hoverT, float activeT) {
    const Color icon = ResolveThemeColor(ThemeToken::IconPrimary);
    const auto base = ToRgb(icon);
    const float lift = std::clamp(hoverT * 0.12f + activeT * 0.08f, 0.0f, 0.2f);
    IconShadeRgb shades{};
    shades.base = base;
    shades.highlight = AdjustBrightness(base, 0.14f + lift);
    shades.shadow = AdjustBrightness(base, -0.18f);
    shades.edge = AdjustBrightness(base, -0.28f);
    return shades;
}

IconBitmap IconBitmap::Create(uint32_t size) {
    IconBitmap bitmap;
    bitmap.width = size;
    bitmap.height = size;
    bitmap.pixels.assign(static_cast<size_t>(size) * size * 4, 0);
    return bitmap;
}

void IconBitmap::SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || x >= static_cast<int>(width) || y >= static_cast<int>(height)) return;
    const size_t idx = (static_cast<size_t>(y) * width + static_cast<size_t>(x)) * 4;
    pixels[idx] = r;
    pixels[idx + 1] = g;
    pixels[idx + 2] = b;
    pixels[idx + 3] = a;
}

void IconBitmap::FillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int py = y; py < y + h; ++py) {
        for (int px = x; px < x + w; ++px) {
            SetPixel(px, py, r, g, b, a);
        }
    }
}

void IconBitmap::FillCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    const int r2 = radius * radius;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                SetPixel(cx + dx, cy + dy, r, g, b, a);
            }
        }
    }
}

void IconBitmap::FillTriangle(
    int x0, int y0, int x1, int y1, int x2, int y2,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const int minY = std::max(0, std::min({ y0, y1, y2 }));
    const int maxY = std::min(static_cast<int>(height) - 1, std::max({ y0, y1, y2 }));
    const int minX = std::max(0, std::min({ x0, x1, x2 }));
    const int maxX = std::min(static_cast<int>(width) - 1, std::max({ x0, x1, x2 }));

    auto edge = [](int ax, int ay, int bx, int by, int cx, int cy) {
        return (cx - ax) * (by - ay) - (bx - ax) * (cy - ay);
    };

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const int w0 = edge(x1, y1, x2, y2, x, y);
            const int w1 = edge(x2, y2, x0, y0, x, y);
            const int w2 = edge(x0, y0, x1, y1, x, y);
            const bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
            const bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);
            if (!(hasNeg && hasPos)) {
                SetPixel(x, y, r, g, b, a);
            }
        }
    }
}

void IconBitmap::FillTriangleGradient(
    int x0, int y0, int x1, int y1, int x2, int y2,
    const std::array<uint8_t, 3>& c0,
    const std::array<uint8_t, 3>& c1,
    const std::array<uint8_t, 3>& c2,
    uint8_t a)
{
    const int minY = std::max(0, std::min({ y0, y1, y2 }));
    const int maxY = std::min(static_cast<int>(height) - 1, std::max({ y0, y1, y2 }));
    const int minX = std::max(0, std::min({ x0, x1, x2 }));
    const int maxX = std::min(static_cast<int>(width) - 1, std::max({ x0, x1, x2 }));

    const float area = static_cast<float>((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0));
    if (std::abs(area) < 0.001f) return;

    auto edge = [](int ax, int ay, int bx, int by, int cx, int cy) {
        return (cx - ax) * (by - ay) - (bx - ax) * (cy - ay);
    };

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const int w0 = edge(x1, y1, x2, y2, x, y);
            const int w1 = edge(x2, y2, x0, y0, x, y);
            const int w2 = edge(x0, y0, x1, y1, x, y);
            const bool hasNeg = (w0 < 0) || (w1 < 0) || (w2 < 0);
            const bool hasPos = (w0 > 0) || (w1 > 0) || (w2 > 0);
            if (hasNeg && hasPos) continue;

            const float l0 = static_cast<float>(w0) / area;
            const float l1 = static_cast<float>(w1) / area;
            const float l2 = static_cast<float>(w2) / area;
            const auto rgb = LerpRgb(LerpRgb(c0, c1, l1 / (l0 + l1 + l2 + 0.0001f)), c2, l2);
            (void)rgb;
            const std::array<uint8_t, 3> color{
                static_cast<uint8_t>(c0[0] * l0 + c1[0] * l1 + c2[0] * l2),
                static_cast<uint8_t>(c0[1] * l0 + c1[1] * l1 + c2[1] * l2),
                static_cast<uint8_t>(c0[2] * l0 + c1[2] * l1 + c2[2] * l2),
            };
            SetPixel(x, y, color[0], color[1], color[2], a);
        }
    }
}

void IconBitmap::DrawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int thickness) {
    const int dx = std::abs(x1 - x0);
    const int dy = std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    int x = x0;
    int y = y0;
    while (true) {
        for (int oy = -thickness / 2; oy <= thickness / 2; ++oy) {
            for (int ox = -thickness / 2; ox <= thickness / 2; ++ox) {
                SetPixel(x + ox, y + oy, r, g, b, a);
            }
        }
        if (x == x1 && y == y1) break;
        const int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

void IconBitmap::FillRoundedRect(int x, int y, int w, int h, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    radius = std::max(0, std::min(radius, std::min(w, h) / 2));
    FillRect(x + radius, y, w - radius * 2, h, r, g, b, a);
    FillRect(x, y + radius, w, h - radius * 2, r, g, b, a);
    FillCircle(x + radius, y + radius, radius, r, g, b, a);
    FillCircle(x + w - radius - 1, y + radius, radius, r, g, b, a);
    FillCircle(x + radius, y + h - radius - 1, radius, r, g, b, a);
    FillCircle(x + w - radius - 1, y + h - radius - 1, radius, r, g, b, a);
}

void IconBitmap::FillRoundedRectVerticalGradient(
    int x, int y, int w, int h, int radius,
    const std::array<uint8_t, 3>& top,
    const std::array<uint8_t, 3>& bottom,
    uint8_t a)
{
    for (int row = 0; row < h; ++row) {
        const float t = h <= 1 ? 0.0f : static_cast<float>(row) / static_cast<float>(h - 1);
        const auto rgb = LerpRgb(top, bottom, t);
        FillRoundedRect(x, y + row, w, 1, radius, rgb[0], rgb[1], rgb[2], a);
    }
}

IconBitmap UpscaleBitmap(const IconBitmap& source, uint32_t targetSize) {
    if (source.width == 0 || source.height == 0 || targetSize == 0) {
        return {};
    }
    if (source.width == targetSize && source.height == targetSize) {
        return source;
    }

    IconBitmap scaled = IconBitmap::Create(targetSize);
    for (uint32_t y = 0; y < targetSize; ++y) {
        for (uint32_t x = 0; x < targetSize; ++x) {
            const uint32_t srcX = (x * source.width) / targetSize;
            const uint32_t srcY = (y * source.height) / targetSize;
            const size_t srcIdx = (static_cast<size_t>(srcY) * source.width + srcX) * 4;
            const size_t dstIdx = (static_cast<size_t>(y) * targetSize + x) * 4;
            scaled.pixels[dstIdx + 0] = source.pixels[srcIdx + 0];
            scaled.pixels[dstIdx + 1] = source.pixels[srcIdx + 1];
            scaled.pixels[dstIdx + 2] = source.pixels[srcIdx + 2];
            scaled.pixels[dstIdx + 3] = source.pixels[srcIdx + 3];
        }
    }
    return scaled;
}

} // namespace WindEffects::Editor::UI
