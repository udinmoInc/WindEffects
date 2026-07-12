#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API AtlasPngLoader {
public:
    [[nodiscard]] static bool LoadRgba8(
        const std::filesystem::path& pngPath,
        std::vector<uint8_t>& outRgba,
        uint32_t& outWidth,
        uint32_t& outHeight,
        std::string& outError);
};

} // namespace WindEffects::Editor::UI
