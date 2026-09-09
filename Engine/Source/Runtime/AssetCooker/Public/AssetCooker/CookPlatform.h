// ==============================================================================
// WindEffects — AssetCooker — CookPlatform
// Public API surface for the AssetCooker module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
