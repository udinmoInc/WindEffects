#include "KindUI/Theming/StyleResolve.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"


namespace we::runtime::kindui {
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

ResolvedStyle BuildFromClass(const StyleClass& cls, const IKindUITheme& theme, float dpiScale) {
    ResolvedStyle style{};
    style.background = theme.ResolveColor(cls.background);
    style.foreground = theme.ResolveColor(cls.foreground);
    style.border = theme.ResolveColor(cls.border);
    style.cornerRadius = ScaleMetric(theme.ResolveMetric(cls.radiusToken), dpiScale);
    style.fontSize = ScaleMetric(theme.ResolveMetric(cls.fontSizeToken), dpiScale);
    style.height = ScaleMetric(theme.ResolveMetric(cls.heightToken), dpiScale);
    style.padding = ScalePadding(theme.ResolvePadding(cls.paddingToken), dpiScale);
    style.bold = cls.bold;
    style.foreground.a *= cls.opacity;
    style.background.a *= cls.opacity;
    return style;
}

} // namespace

ResolvedStyle StyleResolve::FromClass(
    std::string_view className,
    const IKindUITheme& theme,
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
    const IKindUITheme& theme,
    float dpiScale,
    bool hovered,
    bool pressed,
    bool disabled,
    bool selected)
{
    (void)selected;
    ResolvedStyle style = base;
    if (disabled) {
        style.background = theme.ResolveColor(cls.disabledBackground);
        style.foreground = theme.ResolveColor(ColorToken::TextDisabled);
        style.border = theme.ResolveColor(ColorToken::BorderDark);
        return style;
    }
    if (pressed) {
        style.background = theme.ResolveColor(cls.pressedBackground);
    } else if (hovered) {
        style.background = theme.ResolveColor(cls.hoverBackground);
    }
    style.cornerRadius = ScaleMetric(theme.ResolveMetric(cls.radiusToken), dpiScale);
    return style;
}

} // namespace we::runtime::kindui
