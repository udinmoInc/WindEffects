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

// IEC 61966-2-1 — palette HEX is authored in sRGB; convert once before GPU compositing.
[[nodiscard]] inline float SrgbChannelToLinear(float channel) {
    return channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

[[nodiscard]] inline Color SrgbToLinear(Color srgb) {
    return {
        SrgbChannelToLinear(srgb.r),
        SrgbChannelToLinear(srgb.g),
        SrgbChannelToLinear(srgb.b),
        srgb.a
    };
}

// Authoring sRGB → linear RGB for GPU vertex attributes (alpha stays linear).
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
    if (targetFormat == we::rhi::Format::Unknown || IsSrgbSurfaceFormat(targetFormat)) {
        WriteGpuVertexColor(authoringSrgb, out);
        return;
    }
    out[0] = authoringSrgb.r;
    out[1] = authoringSrgb.g;
    out[2] = authoringSrgb.b;
    out[3] = authoringSrgb.a;
}

[[nodiscard]] inline Color ClearColorForTarget(we::rhi::Format targetFormat, Color authoringSrgb) {
    if (targetFormat == we::rhi::Format::Unknown || IsSrgbSurfaceFormat(targetFormat)) {
        return SrgbToLinear(authoringSrgb);
    }
    return authoringSrgb;
}

[[nodiscard]] inline Color OpaqueSurface(Color surface) {
    surface.a = 1.0f;
    return surface;
}

[[nodiscard]] inline bool IsOpaqueAuthoring(Color color) {
    return color.a >= 0.999f;
}

[[nodiscard]] inline Color OpaqueAuthoringColor(Color color) {
    return OpaqueSurface(color);
}

// Discrete state pick — no channel interpolation.
[[nodiscard]] inline Color PickColor(Color a, Color b, float t) {
    return t >= 0.5f ? b : a;
}

} // namespace we::runtime::kindui::ColorSpace

namespace we::runtime::kindui {

inline Color Color::Pick(const Color& other, float t) const {
    return ColorSpace::PickColor(*this, other, t);
}

inline Color Color::Pick(const Color& a, const Color& b, float t) {
    return a.Pick(b, t);
}

} // namespace we::runtime::kindui
