// ==============================================================================
// WindEffects — KindUI — AtlasPngLoader
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace we::runtime::kindui {

class KINDUI_API AtlasPngLoader {
public:
    [[nodiscard]] static bool LoadRgba8(
        const std::filesystem::path& pngPath,
        std::vector<uint8_t>& outRgba,
        uint32_t& outWidth,
        uint32_t& outHeight,
        std::string& outError);
};

} // namespace we::runtime::kindui
