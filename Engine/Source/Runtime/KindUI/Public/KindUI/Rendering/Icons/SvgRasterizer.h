#pragma once

#include "KindUI/Export.h"
#include "KindUI/Rendering/Icons/ISvgRasterizer.h"

namespace we::runtime::kindui::Icons {

// LunaSVG-backed SVG rasterizer (single owner for icon/thumbnail SVG rendering).
class KINDUI_API SvgRasterizer final : public ISvgRasterizer {
public:
    SvgRasterizeResult Rasterize(const SvgRasterizeRequest& request) const override;

    static std::string ResolveAssetPath(const std::string& relativePath);
};

} // namespace we::runtime::kindui::Icons
