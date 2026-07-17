#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Rendering/Icons/ISvgRasterizer.h"

namespace WindEffects::Editor::UI::Icons {

// LunaSVG-backed SVG rasterizer (single owner for icon/thumbnail SVG rendering).
class UI_API SvgRasterizer final : public ISvgRasterizer {
public:
    SvgRasterizeResult Rasterize(const SvgRasterizeRequest& request) const override;

    static std::string ResolveAssetPath(const std::string& relativePath);
};

} // namespace WindEffects::Editor::UI::Icons
