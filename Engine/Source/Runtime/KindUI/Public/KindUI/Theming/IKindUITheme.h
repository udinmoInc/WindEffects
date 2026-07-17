#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Tokens/DesignToken.h"

#include <string_view>

namespace we::runtime::kindui {

// Theme contract: applications supply concrete values for semantic tokens.
// KindUI never contains branding or hardcoded visual values.
class KINDUI_API IKindUITheme {
public:
    virtual ~IKindUITheme() = default;

    [[nodiscard]] virtual std::string_view GetThemeId() const = 0;

    [[nodiscard]] virtual Color ResolveColor(ColorToken token) const = 0;
    [[nodiscard]] virtual float ResolveSpacing(SpacingToken token) const = 0;
    [[nodiscard]] virtual float ResolveRadius(RadiusToken token) const = 0;
    [[nodiscard]] virtual float ResolveFontSize(TypographyToken token) const = 0;
    [[nodiscard]] virtual int ResolveElevation(ElevationToken token) const = 0;
    [[nodiscard]] virtual float ResolveAnimationDuration(AnimationToken token) const = 0;
};

} // namespace we::runtime::kindui
