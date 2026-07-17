#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"

namespace WindEffects::Editor::UI {

// Launcher product theme: GraphiteDark surfaces + professional blue accent.
// Distinct from Editor orange — VS / JetBrains / GitHub Desktop feel.
class UIFRAMEWORK_API LauncherTheme final : public GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Launcher"; }

    [[nodiscard]] Color GetColor(ThemeToken token) const override;
    [[nodiscard]] float GetMetric(ThemeToken token) const override;
};

} // namespace WindEffects::Editor::UI
