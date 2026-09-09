// ==============================================================================
// WindEffects — Icons — PngLoader
// Public API surface for the Icons module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Icons/Export.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace we::runtime::icons {

/// Load a PNG file as RGBA8 pixels. Returns false if the file is missing or unreadable.
ICONS_API bool LoadPngRgba(
    const std::filesystem::path& path,
    std::vector<uint8_t>& outRgba,
    uint32_t& outWidth,
    uint32_t& outHeight);

} // namespace we::runtime::icons
