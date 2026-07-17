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
