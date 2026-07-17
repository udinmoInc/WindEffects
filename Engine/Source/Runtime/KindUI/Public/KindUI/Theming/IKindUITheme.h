#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Tokens/TypographySpec.h"

#include <string_view>

namespace we::runtime::kindui {

// Theme contract: applications supply concrete values for semantic tokens.
// KindUI never contains branding or hardcoded visual values.
class KINDUI_API IKindUITheme {
public:
    virtual ~IKindUITheme() = default;

    [[nodiscard]] virtual std::string_view GetThemeId() const = 0;

    [[nodiscard]] virtual Color ResolveColor(ColorToken token) const = 0;
    [[nodiscard]] virtual float ResolveMetric(MetricToken token) const = 0;
    [[nodiscard]] virtual Margin ResolvePadding(PaddingToken token) const = 0;
    [[nodiscard]] virtual float ResolveSpacing(SpacingToken token) const = 0;
    [[nodiscard]] virtual float ResolveRadius(RadiusToken token) const = 0;
    [[nodiscard]] virtual float ResolveFontSize(TypographyToken token) const = 0;
    /// Full typography resolution (size, weight, color, line height) for a semantic role.
    [[nodiscard]] virtual TypographySpec ResolveTypography(TypographyToken token) const;
    [[nodiscard]] virtual int ResolveElevation(ElevationToken token) const = 0;
    [[nodiscard]] virtual float ResolveAnimationDuration(AnimationToken token) const = 0;

    [[nodiscard]] virtual Color InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const;
    [[nodiscard]] virtual Color IconForState(bool hovered, bool active = false) const;
    [[nodiscard]] virtual Color TextForState(bool hovered, bool active = false) const;
};

} // namespace we::runtime::kindui
