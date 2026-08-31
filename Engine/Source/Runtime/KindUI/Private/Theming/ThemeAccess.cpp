#include "KindUI/Theming/ThemeAccess.h"

#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/TypographySpec.h"

#include <algorithm>


namespace we::runtime::kindui {
namespace {

Color ResolveInteractiveBackgroundImpl(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    using namespace ColorSpace;
    if (selected) {
        return OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }
    const Color base = OpaqueSurface(ResolveColor(surfaceToken));
    const float press = std::clamp(pressAnim, 0.0f, 1.0f);
    if (press > 0.001f) {
        const Color pressed = OpaqueSurface(ResolveColor(ColorToken::PressedBackground));
        return LerpSrgb(base, pressed, press);
    }
    const float hover = std::clamp(hoverAnim, 0.0f, 1.0f);
    if (hover > 0.001f) {
        const Color hoverColor = OpaqueSurface(ResolveColor(ColorToken::HoverBackground));
        return LerpSrgb(base, hoverColor, hover);
    }
    return Color::Transparent();
}

} // namespace

IKindUITheme& ResolveDefaultTheme() {
    return ThemeManager::Get().Theme();
}

Color ResolveColor(ColorToken token) {
    const Color resolved = ThemeManager::Get().Theme().ResolveColor(token);
    if (UiColorDebug::IsEnabled()) {
        UiColorDebug::Get().TraceResolve(token, resolved);
    }
    return resolved;
}

float ResolveMetric(MetricToken token) {
    return ThemeManager::Get().Theme().ResolveMetric(token);
}

Margin ResolvePadding(PaddingToken token) {
    return ThemeManager::Get().Theme().ResolvePadding(token);
}

float ResolveSpacing(SpacingToken token) {
    return ThemeManager::Get().Theme().ResolveSpacing(token);
}

float ResolveRadius(RadiusToken token) {
    return ThemeManager::Get().Theme().ResolveRadius(token);
}

float ResolveFontSize(TypographyToken token) {
    return ThemeManager::Get().Theme().ResolveFontSize(token);
}

TypographySpec ResolveTypography(TypographyToken token) {
    return ThemeManager::Get().Theme().ResolveTypography(token);
}

float ResolveControlHeight(ControlSize size) {
    switch (size) {
    case ControlSize::Compact:
        return ResolveMetric(MetricToken::ControlHeightCompact);
    case ControlSize::Large:
        return ResolveMetric(MetricToken::ControlHeightLarge);
    case ControlSize::Default:
    default:
        return ResolveMetric(MetricToken::ButtonHeight);
    }
}

Color ResolveInteractiveBackground(float hoverAnim, float pressAnim, bool selected) {
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color ResolveInteractiveBackground(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    return ResolveInteractiveBackgroundImpl(hoverAnim, pressAnim, selected, surfaceToken);
}

Color ResolveTextForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().TextForState(hovered, active);
}

Color ResolveIconForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().IconForState(hovered, active);
}

Color ResolveIconColor(
    IconColorRole role,
    float hoverAnim,
    float pressStrength,
    bool accent)
{
    if (accent || role == IconColorRole::Accent) {
        Color accentColor = ResolveColor(ColorToken::IconAccent);
        if (hoverAnim > 0.01f || pressStrength > 0.01f) {
            const Color hover = ResolveColor(ColorToken::AccentHover);
            accentColor = Color::Lerp(accentColor, hover, std::max(hoverAnim, pressStrength));
        }
        return accentColor;
    }

    if (role == IconColorRole::Disabled) {
        return ResolveColor(ColorToken::IconDisabled);
    }

    Color base = ResolveColor(ColorToken::IconSecondary);
    if (pressStrength > 0.01f) {
        base = Color::Lerp(base, ResolveColor(ColorToken::IconActive), pressStrength);
    } else if (hoverAnim > 0.01f) {
        base = Color::Lerp(base, ResolveColor(ColorToken::IconHover), hoverAnim);
    }
    return base;
}

Color ResolveIconColorForState(bool hovered, bool accent, bool disabled, bool secondary) {
    (void)secondary;
    if (disabled) {
        return ResolveIconColor(IconColorRole::Disabled);
    }
    if (accent) {
        return ResolveIconColor(IconColorRole::Accent, hovered ? 1.0f : 0.0f, 0.0f, true);
    }
    return ResolveIconColor(
        IconColorRole::Secondary,
        hovered ? 1.0f : 0.0f,
        0.0f,
        accent);
}

} // namespace we::runtime::kindui
