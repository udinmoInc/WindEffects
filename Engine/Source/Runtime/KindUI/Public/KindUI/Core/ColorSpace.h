#pragma once

#include "KindUI/Core/Types.h"
#include "RHI/Types.h"

#include <cmath>

namespace we::runtime::kindui::ColorSpace {

// sRGB swapchain targets expect linear shader output; UNORM targets store authored sRGB directly.
[[nodiscard]] inline bool IsSrgbSurfaceFormat(we::rhi::Format format) {
    switch (format) {
    case we::rhi::Format::R8G8B8A8_SRGB:
    case we::rhi::Format::B8G8R8A8_SRGB:
        return true;
    default:
        return false;
    }
}

// IEC 61966-2-1 transfers — matches UE FColor → FLinearColor / FLinearColor → display.
// Authoring (Hex, Palette, StyleColors) lives in sRGB. GPU compositing uses linear RGB.

[[nodiscard]] inline float SrgbChannelToLinear(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

[[nodiscard]] inline float LinearChannelToSrgb(float channel) {
    return channel <= 0.0031308f ? channel * 12.92f : 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f;
}

[[nodiscard]] inline Color SrgbToLinear(Color srgb) {
    return {
        SrgbChannelToLinear(srgb.r),
        SrgbChannelToLinear(srgb.g),
        SrgbChannelToLinear(srgb.b),
        srgb.a
    };
}

[[nodiscard]] inline Color LinearToSrgb(Color linear) {
    return {
        LinearChannelToSrgb(linear.r),
        LinearChannelToSrgb(linear.g),
        LinearChannelToSrgb(linear.b),
        linear.a
    };
}

[[nodiscard]] inline Color LerpLinear(Color a, Color b, float t) {
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// Perceptually correct blend for theme/UI colors stored as sRGB.
[[nodiscard]] inline Color LerpSrgb(Color a, Color b, float t) {
    return LinearToSrgb(LerpLinear(SrgbToLinear(a), SrgbToLinear(b), t));
}

// Convert authoring sRGB to linear RGB for GPU vertex attributes (alpha stays linear).
inline void WriteGpuVertexColor(const Color& authoringSrgb, float out[4]) {
    const Color linear = SrgbToLinear(authoringSrgb);
    out[0] = linear.r;
    out[1] = linear.g;
    out[2] = linear.b;
    out[3] = authoringSrgb.a;
}

inline void WriteGpuVertexColorForTarget(
    we::rhi::Format targetFormat,
    const Color& authoringSrgb,
    float out[4])
{
    if (IsSrgbSurfaceFormat(targetFormat)) {
        WriteGpuVertexColor(authoringSrgb, out);
        return;
    }
    out[0] = authoringSrgb.r;
    out[1] = authoringSrgb.g;
    out[2] = authoringSrgb.b;
    out[3] = authoringSrgb.a;
}

[[nodiscard]] inline Color ClearColorForTarget(we::rhi::Format targetFormat, Color authoringSrgb) {
    return IsSrgbSurfaceFormat(targetFormat) ? authoringSrgb.ToLinear() : authoringSrgb;
}

[[nodiscard]] inline Color OpaqueSurface(Color surface) {
    surface.a = 1.0f;
    return surface;
}

} // namespace we::runtime::kindui::ColorSpace

namespace we::runtime::kindui {

inline Color Color::Lerp(const Color& other, float t) const {
    return ColorSpace::LerpSrgb(*this, other, t);
}

inline Color Color::ToLinear() const {
    return ColorSpace::SrgbToLinear(*this);
}

inline Color Color::ToSrgb() const {
    return ColorSpace::LinearToSrgb(*this);
}

inline Color Color::Lerp(const Color& a, const Color& b, float t) {
    return a.Lerp(b, t);
}

} // namespace we::runtime::kindui
