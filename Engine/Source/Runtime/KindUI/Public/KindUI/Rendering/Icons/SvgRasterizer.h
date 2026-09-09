// ==============================================================================
// WindEffects — KindUI — SvgRasterizer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
