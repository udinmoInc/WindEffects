#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/GraphiteDarkTheme.h"

namespace WindEffects::Editor::UI {

// Editor product theme: GraphiteDark surfaces + orange accent for editing tools.
class UI_API EditorTheme final : public GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Editor"; }

    [[nodiscard]] Color GetColor(ThemeToken token) const override;
};

} // namespace WindEffects::Editor::UI
