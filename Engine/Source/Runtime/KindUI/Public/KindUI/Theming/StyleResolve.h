#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IThemeProvider.h"
#include "KindUI/Theming/StyleClass.h"

#include <string_view>

namespace we::runtime::kindui {

/// Converts declarative StyleClass tokens into concrete ResolvedStyle values.
class KINDUI_API StyleResolve {
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

} // namespace we::runtime::kindui
