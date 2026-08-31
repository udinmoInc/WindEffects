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
