#include "WindEffects/Runtime/UI/Theming/StyleResolve.h"

namespace WindEffects::Editor::UI {
namespace {

float ScaleMetric(float value, float dpiScale) {
    return value * dpiScale;
}

Margin ScalePadding(const Margin& padding, float dpiScale) {
    return Margin{
        ScaleMetric(padding.left, dpiScale),
        ScaleMetric(padding.top, dpiScale),
        ScaleMetric(padding.right, dpiScale),
        ScaleMetric(padding.bottom, dpiScale)
    };
}

ResolvedStyle BuildFromClass(const StyleClass& cls, const IThemeProvider& theme, float dpiScale) {
    ResolvedStyle style{};
    style.background = theme.GetColor(cls.background);
    style.foreground = theme.GetColor(cls.foreground);
    style.border = theme.GetColor(cls.border);
    style.cornerRadius = ScaleMetric(theme.GetMetric(cls.radiusToken), dpiScale);
    style.fontSize = ScaleMetric(theme.GetMetric(cls.fontSizeToken), dpiScale);
    style.height = ScaleMetric(theme.GetMetric(cls.heightToken), dpiScale);
    style.padding = ScalePadding(theme.GetPadding(cls.paddingToken), dpiScale);
    style.bold = cls.bold;
    style.foreground.a *= cls.opacity;
    style.background.a *= cls.opacity;
    return style;
}

} // namespace

ResolvedStyle StyleResolve::FromClass(
    std::string_view className,
    const IThemeProvider& theme,
    float dpiScale)
{
    if (className.empty()) {
        return {};
    }
    const StyleClass resolved = StyleClassRegistry::Get().Resolve(className);
    return BuildFromClass(resolved, theme, dpiScale);
}

ResolvedStyle StyleResolve::ApplyState(
    const ResolvedStyle& base,
    const StyleClass& cls,
    const IThemeProvider& theme,
    float dpiScale,
    bool hovered,
    bool pressed,
    bool disabled,
    bool selected)
{
    (void)selected;
    ResolvedStyle style = base;
    if (disabled) {
        style.background = theme.GetColor(cls.disabledBackground);
        style.foreground = theme.GetColor(ThemeToken::TextDisabled);
        style.border = theme.GetColor(ThemeToken::BorderDark);
        return style;
    }
    if (pressed) {
        style.background = theme.GetColor(cls.pressedBackground);
    } else if (hovered) {
        style.background = theme.GetColor(cls.hoverBackground);
    }
    style.cornerRadius = ScaleMetric(theme.GetMetric(cls.radiusToken), dpiScale);
    return style;
}

} // namespace WindEffects::Editor::UI
