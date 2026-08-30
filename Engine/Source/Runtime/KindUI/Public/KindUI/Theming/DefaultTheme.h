#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"

namespace we::runtime::kindui {

// Alias of GraphiteDark — single canonical palette; distinct theme id for framework defaults.
class KINDUI_API DefaultTheme final : public GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "KindUI.Default"; }
};

} // namespace we::runtime::kindui
