#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"
#include "WindEffects/Runtime/UI/Theming/StyleClass.h"

#include <string_view>

namespace WindEffects::Editor::UI {

/// Converts declarative StyleClass tokens into concrete ResolvedStyle values.
class UI_API StyleResolve {
public:
    [[nodiscard]] static ResolvedStyle FromClass(
        std::string_view className,
        const IThemeProvider& theme,
        float dpiScale);

    [[nodiscard]] static ResolvedStyle ApplyState(
        const ResolvedStyle& base,
        const StyleClass& cls,
        const IThemeProvider& theme,
        float dpiScale,
        bool hovered,
        bool pressed,
        bool disabled,
        bool selected);
};

} // namespace WindEffects::Editor::UI
