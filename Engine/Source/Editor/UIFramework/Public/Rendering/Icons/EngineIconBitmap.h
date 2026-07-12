#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace WindEffects::Editor::UI {

struct IconBitmap {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels;

    static IconBitmap Create(uint32_t size);
    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void FillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void FillCircle(int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void FillTriangleGradient(
        int x0, int y0, int x1, int y1, int x2, int y2,
        const std::array<uint8_t, 3>& c0,
        const std::array<uint8_t, 3>& c1,
        const std::array<uint8_t, 3>& c2,
        uint8_t a = 255);
    void DrawLine(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, int thickness = 1);
    void FillRoundedRect(int x, int y, int w, int h, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    void FillRoundedRectVerticalGradient(
        int x, int y, int w, int h, int radius,
        const std::array<uint8_t, 3>& top,
        const std::array<uint8_t, 3>& bottom,
        uint8_t a = 255);
};

constexpr uint32_t kIconDesignSize = 48;

IconBitmap UpscaleBitmap(const IconBitmap& source, uint32_t targetSize);

struct IconShadeRgb {
    std::array<uint8_t, 3> highlight{};
    std::array<uint8_t, 3> base{};
    std::array<uint8_t, 3> shadow{};
    std::array<uint8_t, 3> edge{};
};

IconShadeRgb BuildIconShades(float hoverT, float activeT);

} // namespace WindEffects::Editor::UI
