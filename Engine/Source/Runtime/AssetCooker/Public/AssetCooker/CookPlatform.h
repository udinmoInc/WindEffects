#pragma once

#include "AssetCooker/Export.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::assetcooker {

enum class CookPlatform : uint32_t {
    Windows = 0,
    Linux,
    Android,
    Custom,
    Count
};

[[nodiscard]] ASSETCOOKER_API std::string_view CookPlatformToString(CookPlatform platform);
[[nodiscard]] ASSETCOOKER_API CookPlatform CookPlatformFromString(std::string_view name);

} // namespace we::runtime::assetcooker
