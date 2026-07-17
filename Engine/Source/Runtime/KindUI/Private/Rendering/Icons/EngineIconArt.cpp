#include "WindEffects/Runtime/UI/Rendering/Icons/EngineIconArt.h"

#include "WindEffects/Runtime/UI/Rendering/Icons/EngineIconBitmap.h"
#include "WindEffects/Runtime/UI/Rendering/IconRenderer.h"
#include "WindEffects/Runtime/UI/Core/DPIContext.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace WindEffects::Editor::UI {
namespace {

using Shade = IconShadeRgb;

std::array<uint8_t, 3> LerpRgbLocal(const std::array<uint8_t, 3>& a, const std::array<uint8_t, 3>& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<uint8_t>(a[0] + (b[0] - a[0]) * t),
        static_cast<uint8_t>(a[1] + (b[1] - a[1]) * t),
        static_cast<uint8_t>(a[2] + (b[2] - a[2]) * t),
    };
}

void RenderIsoCube(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 2;
    bmp.FillTriangle(cx, cy - 12, cx + 13, cy - 4, cx, cy + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillTriangle(cx, cy + 4, cx + 13, cy - 4, cx + 13, cy + 12, s.base[0], s.base[1], s.base[2]);
    bmp.FillTriangle(cx, cy + 4, cx - 13, cy - 4, cx, cy - 12, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.FillTriangle(cx, cy + 4, cx - 13, cy - 4, cx - 13, cy + 12, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.DrawLine(cx, cy - 12, cx + 13, cy - 4, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx + 13, cy - 4, cx + 13, cy + 12, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx, cy + 4, cx + 13, cy + 12, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx, cy - 12, cx - 13, cy - 4, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx - 13, cy - 4, cx - 13, cy + 12, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx, cy + 4, cx - 13, cy + 12, s.edge[0], s.edge[1], s.edge[2]);
}

void RenderSphere(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 1;
    const int r = std::max(8, static_cast<int>(bmp.width) / 2 - 8);
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy > r * r) continue;
            const float nx = static_cast<float>(dx) / static_cast<float>(r);
            const float ny = static_cast<float>(dy) / static_cast<float>(r);
            const float shade = 0.55f + nx * -0.18f + ny * -0.28f;
            const auto rgb = LerpRgbLocal(s.shadow, s.highlight, std::clamp(shade, 0.0f, 1.0f));
            bmp.SetPixel(cx + dx, cy + dy, rgb[0], rgb[1], rgb[2]);
        }
    }
}

void RenderCapsule(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int w = std::max(8, static_cast<int>(bmp.width) / 2 - 10);
    bmp.FillRoundedRectVerticalGradient(cx - w / 2, 10, w, static_cast<int>(bmp.height) - 20, w / 2, s.highlight, s.shadow);
    bmp.DrawLine(cx - w / 2, 12, cx - w / 2, static_cast<int>(bmp.height) - 12, s.edge[0], s.edge[1], s.edge[2]);
    bmp.DrawLine(cx + w / 2, 12, cx + w / 2, static_cast<int>(bmp.height) - 12, s.edge[0], s.edge[1], s.edge[2]);
}

void RenderCylinder(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int w = std::max(10, static_cast<int>(bmp.width) / 2 - 8);
    bmp.FillRect(cx - w / 2, 14, w, static_cast<int>(bmp.height) - 28, s.base[0], s.base[1], s.base[2]);
    bmp.FillCircle(cx, 14, w / 2, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillCircle(cx, static_cast<int>(bmp.height) - 14, w / 2, s.shadow[0], s.shadow[1], s.shadow[2]);
}

void RenderCone(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillTriangle(cx, 8, cx + 14, static_cast<int>(bmp.height) - 10, cx - 14, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2]);
    bmp.FillTriangle(cx, 8, cx + 14, static_cast<int>(bmp.height) - 10, cx, static_cast<int>(bmp.height) - 10, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.FillCircle(cx, static_cast<int>(bmp.height) - 10, 14, s.shadow[0], s.shadow[1], s.shadow[2]);
}

void RenderPlane(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 4;
    bmp.FillTriangle(cx, cy - 10, cx + 18, cy + 2, cx, cy + 14, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillTriangle(cx, cy + 14, cx + 18, cy + 2, cx - 18, cy + 2, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.FillTriangle(cx, cy - 10, cx - 18, cy + 2, cx, cy + 14, s.base[0], s.base[1], s.base[2]);
}

void RenderPointLight(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.FillCircle(cx, cy, 7, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillCircle(cx, cy, 4, s.base[0], s.base[1], s.base[2]);
    for (int i = 0; i < 8; ++i) {
        const float a = static_cast<float>(i) * 0.785398f;
        const int x1 = cx + static_cast<int>(std::cos(a) * 10.0f);
        const int y1 = cy + static_cast<int>(std::sin(a) * 10.0f);
        const int x2 = cx + static_cast<int>(std::cos(a) * 16.0f);
        const int y2 = cy + static_cast<int>(std::sin(a) * 16.0f);
        bmp.DrawLine(x1, y1, x2, y2, s.base[0], s.base[1], s.base[2], 180);
    }
}

void RenderSpotLight(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillCircle(cx, 12, 5, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillTriangle(cx - 14, static_cast<int>(bmp.height) - 8, cx + 14, static_cast<int>(bmp.height) - 8, cx, 18, s.base[0], s.base[1], s.base[2], 120);
    bmp.DrawLine(cx - 14, static_cast<int>(bmp.height) - 8, cx + 14, static_cast<int>(bmp.height) - 8, s.edge[0], s.edge[1], s.edge[2]);
}

void RenderDirectionalLight(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.FillCircle(cx, cy, 9, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillCircle(cx, cy, 6, s.base[0], s.base[1], s.base[2]);
    for (int i = 0; i < 8; ++i) {
        const float a = static_cast<float>(i) * 0.785398f;
        bmp.DrawLine(
            cx + static_cast<int>(std::cos(a) * 12.0f),
            cy + static_cast<int>(std::sin(a) * 12.0f),
            cx + static_cast<int>(std::cos(a) * 18.0f),
            cy + static_cast<int>(std::sin(a) * 18.0f),
            s.highlight[0], s.highlight[1], s.highlight[2], 220, 2);
    }
}

void RenderCamera(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillRoundedRectVerticalGradient(cx - 14, 14, 28, 18, 4, s.base, s.shadow);
    bmp.FillCircle(cx + 10, 22, 8, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillCircle(cx + 10, 22, 4, s.shadow[0], s.shadow[1], s.shadow[2]);
}

void RenderCharacter(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillCircle(cx, 12, 6, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.FillRoundedRectVerticalGradient(cx - 8, 20, 16, 14, 4, s.base, s.shadow);
    bmp.DrawLine(cx, 34, cx - 10, static_cast<int>(bmp.height) - 8, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.DrawLine(cx, 34, cx + 10, static_cast<int>(bmp.height) - 8, s.base[0], s.base[1], s.base[2], 255, 2);
}

void RenderEmptyActor(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.DrawLine(cx - 12, cy, cx + 12, cy, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.DrawLine(cx, cy - 12, cx, cy + 12, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.FillCircle(cx, cy, 3, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderBlueprint(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(8, 12, 14, 10, 3, s.base[0], s.base[1], s.base[2]);
    bmp.FillRoundedRect(26, 26, 14, 10, 3, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(22, 17, 26, 31, s.highlight[0], s.highlight[1], s.highlight[2], 255, 2);
}

void RenderVolume(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 2;
    const int h = 12;
    const int w = 14;
    auto box = [&](int ox, int oy) {
        bmp.DrawLine(cx - w + ox, cy - h + oy, cx + w + ox, cy - h + oy, s.base[0], s.base[1], s.base[2]);
        bmp.DrawLine(cx + w + ox, cy - h + oy, cx + w + ox, cy + h + oy, s.base[0], s.base[1], s.base[2]);
        bmp.DrawLine(cx + w + ox, cy + h + oy, cx - w + ox, cy + h + oy, s.base[0], s.base[1], s.base[2]);
        bmp.DrawLine(cx - w + ox, cy + h + oy, cx - w + ox, cy - h + oy, s.base[0], s.base[1], s.base[2]);
    };
    box(-4, -4);
    box(4, 4);
    bmp.DrawLine(cx - w - 4, cy - h - 4, cx - w + 4, cy - h + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.DrawLine(cx + w - 4, cy - h - 4, cx + w + 4, cy - h + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.DrawLine(cx + w - 4, cy + h - 4, cx + w + 4, cy + h + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderParticle(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    for (int i = 0; i < 10; ++i) {
        const float a = static_cast<float>(i) * 0.62f;
        const int r = 4 + (i % 3) * 2;
        bmp.FillCircle(cx + static_cast<int>(std::cos(a) * 12.0f), cy + static_cast<int>(std::sin(a) * 10.0f), r,
            s.highlight[0], s.highlight[1], s.highlight[2], static_cast<uint8_t>(180 - i * 10));
    }
}

void RenderAudio(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillCircle(cx - 4, static_cast<int>(bmp.height) / 2 + 4, 4, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(cx, 14, cx, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.DrawLine(cx, 18, cx + 10, 24, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.DrawLine(cx + 10, 24, cx + 10, static_cast<int>(bmp.height) - 14, s.base[0], s.base[1], s.base[2], 255, 2);
}

void RenderLandscape(IconBitmap& bmp, const Shade& s) {
    const int w = static_cast<int>(bmp.width);
    const int h = static_cast<int>(bmp.height);
    bmp.FillTriangle(4, h - 8, w - 4, h - 8, w / 2, 14, s.base[0], s.base[1], s.base[2]);
    bmp.FillTriangle(w / 2, 14, w - 4, h - 8, w / 2 + 8, h - 8, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderSearch(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2 - 2;
    const int cy = static_cast<int>(bmp.height) / 2 - 2;
    bmp.FillCircle(cx, cy, 9, s.base[0], s.base[1], s.base[2], 0);
    bmp.DrawLine(cx + 6, cy + 6, cx + 14, cy + 14, s.highlight[0], s.highlight[1], s.highlight[2], 255, 2);
    for (int dy = -9; dy <= 9; ++dy) {
        for (int dx = -9; dx <= 9; ++dx) {
            const int dist = dx * dx + dy * dy;
            if (dist >= 49 && dist <= 81) {
                bmp.SetPixel(cx + dx, cy + dy, s.base[0], s.base[1], s.base[2]);
            }
        }
    }
}

void RenderSettings(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.FillCircle(cx, cy, 6, s.shadow[0], s.shadow[1], s.shadow[2]);
    for (int i = 0; i < 6; ++i) {
        const float a = static_cast<float>(i) * 1.0472f;
        const int x1 = cx + static_cast<int>(std::cos(a) * 9.0f);
        const int y1 = cy + static_cast<int>(std::sin(a) * 9.0f);
        const int x2 = cx + static_cast<int>(std::cos(a) * 16.0f);
        const int y2 = cy + static_cast<int>(std::sin(a) * 16.0f);
        bmp.DrawLine(x1, y1, x2, y2, s.base[0], s.base[1], s.base[2], 255, 3);
    }
}

void RenderFilter(IconBitmap& bmp, const Shade& s) {
    bmp.FillTriangle(10, 10, static_cast<int>(bmp.width) - 10, 10, static_cast<int>(bmp.width) / 2, 22, s.base[0], s.base[1], s.base[2]);
    bmp.FillRect(static_cast<int>(bmp.width) / 2 - 2, 22, 4, static_cast<int>(bmp.height) - 18, s.base[0], s.base[1], s.base[2]);
}

void RenderStar(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    for (int i = 0; i < 5; ++i) {
        const float a0 = static_cast<float>(i) * 1.256f - 1.5708f;
        const float a1 = a0 + 0.628f;
        bmp.FillTriangle(
            cx, cy,
            cx + static_cast<int>(std::cos(a0) * 14.0f), cy + static_cast<int>(std::sin(a0) * 14.0f),
            cx + static_cast<int>(std::cos(a1) * 6.0f), cy + static_cast<int>(std::sin(a1) * 6.0f),
            s.highlight[0], s.highlight[1], s.highlight[2]);
    }
}

void RenderNote(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(10, 8, static_cast<int>(bmp.width) - 20, static_cast<int>(bmp.height) - 16, 4, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(14, 16, static_cast<int>(bmp.width) - 14, 16, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.DrawLine(14, 22, static_cast<int>(bmp.width) - 20, 22, s.shadow[0], s.shadow[1], s.shadow[2]);
}

void RenderWidget(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(8, 12, static_cast<int>(bmp.width) - 16, static_cast<int>(bmp.height) - 24, 4, s.base[0], s.base[1], s.base[2]);
    bmp.FillRect(12, 16, static_cast<int>(bmp.width) - 24, 4, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderPlay(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2 + 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.FillTriangle(cx - 8, cy - 12, cx - 8, cy + 12, cx + 12, cy, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderPause(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    bmp.FillRoundedRect(cx - 10, cy - 12, 6, 24, 2, s.base[0], s.base[1], s.base[2]);
    bmp.FillRoundedRect(cx + 4, cy - 12, 6, 24, 2, s.base[0], s.base[1], s.base[2]);
}

void RenderStop(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(10, 10, static_cast<int>(bmp.width) - 20, static_cast<int>(bmp.height) - 20, 3, s.base[0], s.base[1], s.base[2]);
}

void RenderRefresh(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2;
    for (int i = 0; i < 24; ++i) {
        const float a = static_cast<float>(i) / 24.0f * 4.8f + 0.4f;
        const int x = cx + static_cast<int>(std::cos(a) * 11.0f);
        const int y = cy + static_cast<int>(std::sin(a) * 11.0f);
        bmp.SetPixel(x, y, s.base[0], s.base[1], s.base[2]);
    }
    bmp.FillTriangle(cx + 10, cy - 12, cx + 16, cy - 6, cx + 8, cy - 4, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderSave(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(10, 8, static_cast<int>(bmp.width) - 20, static_cast<int>(bmp.height) - 12, 3, s.base[0], s.base[1], s.base[2]);
    bmp.FillRect(14, 8, static_cast<int>(bmp.width) - 28, 8, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.FillRoundedRect(14, 20, static_cast<int>(bmp.width) - 28, static_cast<int>(bmp.height) - 32, 2, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderUndo(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2 + 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 2;
    for (int i = 0; i < 20; ++i) {
        const float a = static_cast<float>(i) / 20.0f * 2.4f + 2.0f;
        const int x = cx + static_cast<int>(std::cos(a) * 12.0f);
        const int y = cy + static_cast<int>(std::sin(a) * 12.0f);
        bmp.SetPixel(x, y, s.base[0], s.base[1], s.base[2]);
    }
    bmp.FillTriangle(cx - 12, cy - 4, cx - 4, cy - 12, cx - 4, cy + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderRedo(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2 - 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 2;
    for (int i = 0; i < 20; ++i) {
        const float a = static_cast<float>(i) / 20.0f * 2.4f - 0.8f;
        const int x = cx + static_cast<int>(std::cos(a) * 12.0f);
        const int y = cy + static_cast<int>(std::sin(a) * 12.0f);
        bmp.SetPixel(x, y, s.base[0], s.base[1], s.base[2]);
    }
    bmp.FillTriangle(cx + 12, cy - 4, cx + 4, cy - 12, cx + 4, cy + 4, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderDelete(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(12, 14, static_cast<int>(bmp.width) - 24, static_cast<int>(bmp.height) - 22, 3, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(10, 12, static_cast<int>(bmp.width) - 10, 12, s.shadow[0], s.shadow[1], s.shadow[2], 255, 2);
    bmp.DrawLine(static_cast<int>(bmp.width) / 2 - 4, 18, static_cast<int>(bmp.width) / 2 - 4, static_cast<int>(bmp.height) - 10, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.DrawLine(static_cast<int>(bmp.width) / 2 + 4, 18, static_cast<int>(bmp.width) / 2 + 4, static_cast<int>(bmp.height) - 10, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderDuplicate(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(16, 10, static_cast<int>(bmp.width) - 24, static_cast<int>(bmp.height) - 24, 3, s.shadow[0], s.shadow[1], s.shadow[2]);
    bmp.FillRoundedRect(8, 16, static_cast<int>(bmp.width) - 24, static_cast<int>(bmp.height) - 24, 3, s.base[0], s.base[1], s.base[2]);
}

void RenderRename(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(8, 12, static_cast<int>(bmp.width) - 16, static_cast<int>(bmp.height) - 24, 3, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(12, 20, static_cast<int>(bmp.width) - 12, 20, s.highlight[0], s.highlight[1], s.highlight[2]);
    bmp.DrawLine(static_cast<int>(bmp.width) - 14, static_cast<int>(bmp.height) - 16, static_cast<int>(bmp.width) - 8, static_cast<int>(bmp.height) - 10, s.highlight[0], s.highlight[1], s.highlight[2], 255, 2);
}

void RenderImport(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillTriangle(cx, static_cast<int>(bmp.height) - 10, cx - 10, static_cast<int>(bmp.height) - 24, cx + 10, static_cast<int>(bmp.height) - 24, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(cx, 10, cx, static_cast<int>(bmp.height) - 22, s.base[0], s.base[1], s.base[2], 255, 2);
}

void RenderExport(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillTriangle(cx, 10, cx - 10, 24, cx + 10, 24, s.base[0], s.base[1], s.base[2]);
    bmp.DrawLine(cx, 22, cx, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2], 255, 2);
}

void RenderFolder(IconBitmap& bmp, const Shade& s) {
    bmp.FillRoundedRect(8, 16, static_cast<int>(bmp.width) - 16, static_cast<int>(bmp.height) - 22, 3, s.base[0], s.base[1], s.base[2]);
    bmp.FillRoundedRect(8, 12, 14, 8, 2, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderScene(IconBitmap& bmp, const Shade& s) {
    RenderIsoCube(bmp, s);
    bmp.FillCircle(static_cast<int>(bmp.width) - 12, 12, 3, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderLevel(IconBitmap& bmp, const Shade& s) {
    bmp.FillRect(8, static_cast<int>(bmp.height) - 12, static_cast<int>(bmp.width) - 16, 4, s.base[0], s.base[1], s.base[2]);
    bmp.FillTriangle(8, static_cast<int>(bmp.height) - 12, static_cast<int>(bmp.width) / 2, 10, static_cast<int>(bmp.width) - 8, static_cast<int>(bmp.height) - 12, s.highlight[0], s.highlight[1], s.highlight[2]);
}

void RenderFont(IconBitmap& bmp, const Shade& s) {
    bmp.DrawLine(14, 12, 14, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2], 255, 3);
    bmp.DrawLine(14, 12, static_cast<int>(bmp.width) - 12, 12, s.base[0], s.base[1], s.base[2], 255, 2);
    bmp.DrawLine(static_cast<int>(bmp.width) - 12, 12, static_cast<int>(bmp.width) - 12, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2], 255, 2);
}

void RenderTorus(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    const int cy = static_cast<int>(bmp.height) / 2 + 1;
    for (int dy = -12; dy <= 12; ++dy) {
        for (int dx = -12; dx <= 12; ++dx) {
            const int dist = dx * dx + dy * dy;
            if (dist >= 36 && dist <= 144) {
                const float shade = 0.55f + dx * -0.02f + dy * -0.03f;
                const auto rgb = LerpRgbLocal(s.shadow, s.highlight, std::clamp(shade, 0.0f, 1.0f));
                bmp.SetPixel(cx + dx, cy + dy, rgb[0], rgb[1], rgb[2]);
            }
        }
    }
}

void RenderPyramid(IconBitmap& bmp, const Shade& s) {
    const int cx = static_cast<int>(bmp.width) / 2;
    bmp.FillTriangle(cx, 8, cx + 14, static_cast<int>(bmp.height) - 10, cx - 14, static_cast<int>(bmp.height) - 10, s.base[0], s.base[1], s.base[2]);
    bmp.FillTriangle(cx, 8, cx, static_cast<int>(bmp.height) - 10, cx - 14, static_cast<int>(bmp.height) - 10, s.shadow[0], s.shadow[1], s.shadow[2]);
}

IconBitmap RenderIconBitmap(std::string_view iconName, uint32_t targetSizePx, float hoverAnim, float activeAnim) {
    IconBitmap bmp = IconBitmap::Create(kIconDesignSize);
    const Shade shades = BuildIconShades(hoverAnim, activeAnim);

    static const std::unordered_map<std::string, void(*)(IconBitmap&, const Shade&)> kRenderers = {
        { EngineIcons::Cube, RenderIsoCube },
        { EngineIcons::Sphere, RenderSphere },
        { EngineIcons::Capsule, RenderCapsule },
        { EngineIcons::Cylinder, RenderCylinder },
        { EngineIcons::Cone, RenderCone },
        { EngineIcons::Plane, RenderPlane },
        { EngineIcons::Pyramid, RenderPyramid },
        { EngineIcons::Torus, RenderTorus },
        { EngineIcons::Arrow, RenderCone },
        { EngineIcons::LightPoint, RenderPointLight },
        { EngineIcons::LightSpot, RenderSpotLight },
        { EngineIcons::LightDirectional, RenderDirectionalLight },
        { EngineIcons::LightRect, RenderPlane },
        { EngineIcons::LightSky, RenderDirectionalLight },
        { EngineIcons::Camera, RenderCamera },
        { EngineIcons::CineCamera, RenderCamera },
        { EngineIcons::Character, RenderCharacter },
        { EngineIcons::Pawn, RenderCharacter },
        { EngineIcons::EmptyActor, RenderEmptyActor },
        { EngineIcons::Blueprint, RenderBlueprint },
        { EngineIcons::TriggerVolume, RenderVolume },
        { EngineIcons::PhysicsVolume, RenderVolume },
        { EngineIcons::NavVolume, RenderVolume },
        { EngineIcons::Particle, RenderParticle },
        { EngineIcons::Effect, RenderParticle },
        { EngineIcons::Decal, RenderPlane },
        { EngineIcons::ReflectionProbe, RenderSphere },
        { EngineIcons::Landscape, RenderLandscape },
        { EngineIcons::Terrain, RenderLandscape },
        { EngineIcons::Foliage, RenderLandscape },
        { EngineIcons::Sky, RenderDirectionalLight },
        { EngineIcons::Water, RenderPlane },
        { EngineIcons::Clouds, RenderParticle },
        { EngineIcons::Fog, RenderParticle },
        { EngineIcons::Audio, RenderAudio },
        { EngineIcons::Material, RenderPlane },
        { EngineIcons::Texture, RenderPlane },
        { EngineIcons::StaticMesh, RenderIsoCube },
        { EngineIcons::SkeletalMesh, RenderCharacter },
        { EngineIcons::Animation, RenderPlay },
        { EngineIcons::Search, RenderSearch },
        { EngineIcons::Settings, RenderSettings },
        { EngineIcons::Filter, RenderFilter },
        { EngineIcons::Star, RenderStar },
        { EngineIcons::Note, RenderNote },
        { EngineIcons::Widget, RenderWidget },
        { EngineIcons::AI, RenderBlueprint },
        { EngineIcons::Navigation, RenderVolume },
        { EngineIcons::Trigger, RenderVolume },
        { EngineIcons::Font, RenderFont },
        { EngineIcons::Folder, RenderFolder },
        { EngineIcons::Scene, RenderScene },
        { EngineIcons::Level, RenderLevel },
        { EngineIcons::Save, RenderSave },
        { EngineIcons::Undo, RenderUndo },
        { EngineIcons::Redo, RenderRedo },
        { EngineIcons::Refresh, RenderRefresh },
        { EngineIcons::Delete, RenderDelete },
        { EngineIcons::Duplicate, RenderDuplicate },
        { EngineIcons::Rename, RenderRename },
        { EngineIcons::Import, RenderImport },
        { EngineIcons::Export, RenderExport },
        { EngineIcons::Play, RenderPlay },
        { EngineIcons::Pause, RenderPause },
        { EngineIcons::Stop, RenderStop },
        { EngineIcons::CategoryBasic, RenderEmptyActor },
        { EngineIcons::CategoryGeometry, RenderIsoCube },
        { EngineIcons::CategoryLights, RenderPointLight },
        { EngineIcons::CategoryAll, RenderNote },
    };

    const std::string key(iconName);
    if (const auto it = kRenderers.find(key); it != kRenderers.end()) {
        it->second(bmp, shades);
    } else {
        RenderEmptyActor(bmp, shades);
    }

    if (targetSizePx > kIconDesignSize) {
        return UpscaleBitmap(bmp, targetSizePx);
    }
    return bmp;
}

} // namespace

bool EngineIcons::IsEngineIcon(std::string_view iconName) {
    return iconName.size() > 3 && iconName.substr(0, 3) == "we/";
}

EngineIconArt& EngineIconArt::Get() {
    static EngineIconArt instance;
    return instance;
}

void EngineIconArt::Initialize(IconRenderer* renderer) {
    m_Renderer = renderer;
}

void EngineIconArt::InvalidateCache() {
    m_Cache.clear();
}

bool EngineIconArt::IsEngineIcon(std::string_view iconName) const {
    return EngineIcons::IsEngineIcon(iconName);
}

we::rhi::RHIDescriptorSetHandle EngineIconArt::GetTexture(std::string_view iconName, uint32_t displaySizePx, float hoverAnim, float activeAnim) const {
    if (!m_Renderer || displaySizePx == 0) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const float dpi = std::max(1.0f, DPIContext::GetScale());
    const uint32_t rasterSize = std::max(kIconDesignSize, static_cast<uint32_t>(std::ceil(static_cast<float>(displaySizePx) * dpi)));

    const int hoverBucket = static_cast<int>(std::round(hoverAnim * 4.0f));
    const int activeBucket = static_cast<int>(std::round(activeAnim * 4.0f));
    const std::string key = std::string(iconName) + "_" + std::to_string(rasterSize) + "_h" + std::to_string(hoverBucket) + "_a" + std::to_string(activeBucket);

    if (const auto it = m_Cache.find(key); it != m_Cache.end()) {
        return it->second;
    }

    const IconBitmap bitmap = RenderIconBitmap(iconName, rasterSize, hoverAnim, activeAnim);
    if (bitmap.pixels.empty()) return we::rhi::RHIDescriptorSetHandle::Invalid;

    const we::rhi::RHIDescriptorSetHandle texture = m_Renderer->CreateTextureFromBitmap(bitmap.pixels, bitmap.width, bitmap.height);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        m_Cache[key] = texture;
    }
    return texture;
}

void EngineIconArt::Paint(
    PaintContext& context,
    std::string_view iconName,
    const Rect& bounds,
    float hoverAnim,
    float activeAnim) const
{
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) return;

    const uint32_t sizePx = static_cast<uint32_t>(std::max(bounds.width, bounds.height));
    const we::rhi::RHIDescriptorSetHandle texture = GetTexture(iconName, sizePx, hoverAnim, activeAnim);
    if ((texture != we::rhi::RHIDescriptorSetHandle::Invalid)) {
        context.DrawColorTexture(bounds, texture);
    }
}

} // namespace WindEffects::Editor::UI
