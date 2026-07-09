#pragma once

#include "Application/Export.h"
#include "Rendering/Icons/ISvgRasterizer.h"

namespace we::UI::Icons {

// LunaSVG-backed SVG rasterizer (single owner for icon/thumbnail SVG rendering).
class APPLICATION_API SvgRasterizer final : public ISvgRasterizer {
public:
    SvgRasterizeResult Rasterize(const SvgRasterizeRequest& request) const override;

    static std::string ResolveAssetPath(const std::string& relativePath);
};

} // namespace we::UI::Icons
