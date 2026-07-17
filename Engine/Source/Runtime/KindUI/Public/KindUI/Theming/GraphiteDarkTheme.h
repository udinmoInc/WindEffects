#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Theming/ResolvedStyle.h"

#include <memory>

namespace we::runtime::kindui {

class KINDUI_API GraphiteDarkTheme : public IKindUITheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "GraphiteDark"; }

    [[nodiscard]] Color ResolveColor(ColorToken token) const override;
    [[nodiscard]] float ResolveMetric(MetricToken token) const override;
    [[nodiscard]] Margin ResolvePadding(PaddingToken token) const override;
    [[nodiscard]] float ResolveSpacing(SpacingToken token) const override;
    [[nodiscard]] float ResolveRadius(RadiusToken token) const override;
    [[nodiscard]] float ResolveFontSize(TypographyToken token) const override;
    [[nodiscard]] int ResolveElevation(ElevationToken token) const override;
    [[nodiscard]] float ResolveAnimationDuration(AnimationToken token) const override;

    [[nodiscard]] Color InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const override;
    [[nodiscard]] Color IconForState(bool hovered, bool active = false) const override;
    [[nodiscard]] Color TextForState(bool hovered, bool active = false) const override;
};

class StyleResolver final : public IStyleResolver {
public:
    explicit StyleResolver(std::shared_ptr<IKindUITheme> theme);

    [[nodiscard]] ResolvedStyle Resolve(StyleRole role) const override;
    [[nodiscard]] ResolvedStyle ResolveClass(std::string_view className) const override;
    [[nodiscard]] float Scaled(float logicalValue) const override;
    [[nodiscard]] float GetDpiScale() const override { return m_DpiScale; }
    void SetDpiScale(float scale) override;

private:
    std::shared_ptr<IKindUITheme> m_Theme;
    float m_DpiScale = 1.0f;
};

} // namespace we::runtime::kindui
