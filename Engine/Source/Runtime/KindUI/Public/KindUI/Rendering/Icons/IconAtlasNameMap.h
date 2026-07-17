#pragma once

#include "KindUI/Export.h"

#include <string>
#include <string_view>

namespace we::runtime::kindui {

class KINDUI_API IconAtlasNameMap {
public:
    [[nodiscard]] static std::string ResolveRegionName(std::string_view logicalName);
    [[nodiscard]] static bool IsAtlasBackedName(std::string_view logicalName);
};

} // namespace we::runtime::kindui
