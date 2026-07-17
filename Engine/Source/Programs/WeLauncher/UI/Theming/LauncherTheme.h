#pragma once

#include "WindEffects/Runtime/UI/Theming/GraphiteDarkTheme.h"

namespace we::programs::welauncher {

/// Launcher product theme (blue accent). Lives with the Launcher app, not the Runtime UI module.
class LauncherTheme final : public WindEffects::Editor::UI::GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Launcher"; }

    [[nodiscard]] WindEffects::Editor::UI::Color GetColor(WindEffects::Editor::UI::ThemeToken token) const override;
    [[nodiscard]] float GetMetric(WindEffects::Editor::UI::ThemeToken token) const override;
};

} // namespace we::programs::welauncher
