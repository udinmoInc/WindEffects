#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IKindUITheme.h"

namespace we::runtime::kindui {

// Framework-default theme values. Applications override through their own themes.
class KINDUI_API DefaultTheme final : public IKindUITheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "KindUI.Default"; }

    [[nodiscard]] Color ResolveColor(ColorToken token) const override;
    [[nodiscard]] float ResolveMetric(MetricToken token) const override;
    [[nodiscard]] Margin ResolvePadding(PaddingToken token) const override;
    [[nodiscard]] float ResolveSpacing(SpacingToken token) const override;
    [[nodiscard]] float ResolveRadius(RadiusToken token) const override;
    [[nodiscard]] float ResolveFontSize(TypographyToken token) const override;
    [[nodiscard]] int ResolveElevation(ElevationToken token) const override;
    [[nodiscard]] float ResolveAnimationDuration(AnimationToken token) const override;
};

} // namespace we::runtime::kindui
