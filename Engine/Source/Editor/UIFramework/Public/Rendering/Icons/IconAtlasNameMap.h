#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <string>
#include <string_view>

namespace WindEffects::Editor::UI {

class UIFRAMEWORK_API IconAtlasNameMap {
public:
    [[nodiscard]] static std::string ResolveRegionName(std::string_view logicalName);
    [[nodiscard]] static bool IsAtlasBackedName(std::string_view logicalName);
};

} // namespace WindEffects::Editor::UI
