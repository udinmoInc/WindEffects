#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include <string>
#include <string_view>

namespace WindEffects::Editor::UI {

class UI_API IconAtlasNameMap {
public:
    [[nodiscard]] static std::string ResolveRegionName(std::string_view logicalName);
    [[nodiscard]] static bool IsAtlasBackedName(std::string_view logicalName);
};

} // namespace WindEffects::Editor::UI
