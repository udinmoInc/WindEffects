#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/GraphiteDarkTheme.h"

namespace WindEffects::Editor::UI {

// Launcher product theme: GraphiteDark surfaces + professional blue accent.
// Distinct from Editor orange — VS / JetBrains / GitHub Desktop feel.
class UI_API LauncherTheme final : public GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Launcher"; }

    [[nodiscard]] Color GetColor(ThemeToken token) const override;
    [[nodiscard]] float GetMetric(ThemeToken token) const override;
};

} // namespace WindEffects::Editor::UI
