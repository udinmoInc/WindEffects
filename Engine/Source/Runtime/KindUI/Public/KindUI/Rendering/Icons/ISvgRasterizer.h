#pragma once

#include "KindUI/Export.h"

#include "KindUI/Core/Geometry.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::kindui::Icons {

struct SvgRasterizeRequest {
    std::string svgPath;
    uint32_t width = 0;
    uint32_t height = 0;
    Color tint = Color::White();
    float strokeWidth = 0.0f;
    bool applyTint = true;
};

struct SvgRasterizeResult {
    std::vector<uint8_t> rgba;
    uint32_t width = 0;
    uint32_t height = 0;
    bool success = false;
};

// Backend-independent SVG rasterization interface.
class KINDUI_API ISvgRasterizer {
public:
    virtual ~ISvgRasterizer() = default;
    virtual SvgRasterizeResult Rasterize(const SvgRasterizeRequest& request) const = 0;
};

} // namespace we::runtime::kindui::Icons
