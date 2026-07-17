#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::services {

// Editor product theme: GraphiteDark surfaces + orange accent for editing tools.
class UIFRAMEWORK_API EditorTheme final : public we::runtime::kindui::GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Editor"; }

    [[nodiscard]] we::runtime::kindui::Color ResolveColor(we::runtime::kindui::ColorToken token) const override;
};

} // namespace we::editor::services
