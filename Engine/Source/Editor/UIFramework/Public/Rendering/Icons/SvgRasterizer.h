#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Rendering/Icons/ISvgRasterizer.h"

namespace WindEffects::Editor::UI::Icons {

// LunaSVG-backed SVG rasterizer (single owner for icon/thumbnail SVG rendering).
class UIFRAMEWORK_API SvgRasterizer final : public ISvgRasterizer {
public:
    SvgRasterizeResult Rasterize(const SvgRasterizeRequest& request) const override;

    static std::string ResolveAssetPath(const std::string& relativePath);
};

} // namespace WindEffects::Editor::UI::Icons
