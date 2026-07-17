#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Theming/IThemeProvider.h"

#include <memory>

namespace WindEffects::Editor::UI {

class UI_API GraphiteDarkTheme : public IThemeProvider {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "GraphiteDark"; }

    [[nodiscard]] Color GetColor(ThemeToken token) const override;
    [[nodiscard]] float GetMetric(ThemeToken token) const override;
    [[nodiscard]] Margin GetPadding(ThemeToken token) const override;

    [[nodiscard]] Color InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const override;
    [[nodiscard]] Color IconForState(bool hovered, bool active = false) const override;
    [[nodiscard]] Color TextForState(bool hovered, bool active = false) const override;
};

class StyleResolver final : public IStyleResolver {
public:
    explicit StyleResolver(std::shared_ptr<IThemeProvider> theme);

    [[nodiscard]] ResolvedStyle Resolve(StyleRole role) const override;
    [[nodiscard]] ResolvedStyle ResolveClass(std::string_view className) const override;
    [[nodiscard]] float Scaled(float logicalValue) const override;
    [[nodiscard]] float GetDpiScale() const override { return m_DpiScale; }
    void SetDpiScale(float scale) override;

private:
    std::shared_ptr<IThemeProvider> m_Theme;
    float m_DpiScale = 1.0f;
};

} // namespace WindEffects::Editor::UI
