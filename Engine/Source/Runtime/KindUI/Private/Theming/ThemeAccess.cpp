#include "KindUI/Theming/ThemeAccess.h"

#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Theming/PaletteRuntime.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/TypographySpec.h"

#include <algorithm>


namespace we::runtime::kindui {
namespace {

bool IsCompositeColorToken(ColorToken token) {
    switch (token) {
    case ColorToken::TooltipBackground:
    case ColorToken::DragGhostBackground:
    case ColorToken::ActiveTabLine:
    case ColorToken::SelectionHighlight:
    case ColorToken::HighlightSubtle:
    case ColorToken::ModalScrim:
    case ColorToken::ShadowPopup:
    case ColorToken::ShadowSubtle:
    case ColorToken::ShadowOverlay:
    case ColorToken::ShadowColor:
    case ColorToken::ContentBrowserFolderShadow:
    case ColorToken::InputInsetInner:
    case ColorToken::InputInsetOuter:
    case ColorToken::IconContactShadow:
        return true;
    default:
        return false;
    }
}

constexpr float kHoverMix = 0.62f;
constexpr float kPressMix = 0.50f;

float Clamp01(float t) {
    return std::clamp(t, 0.0f, 1.0f);
}

Color MixInteractiveSurfaceImpl(
    Color base,
    float hoverAnim,
    float pressAnim,
    bool selected,
    bool disabled)
{
    if (disabled) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::DisabledBackground));
    }
    if (selected) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }

    const float hover = Clamp01(hoverAnim);
    const float press = Clamp01(pressAnim);

    if (base.a < 0.01f) {
        if (press > 0.001f && press >= hover) {
            Color fill = ColorSpace::OpaqueSurface(ResolveColor(ColorToken::PressedBackground));
            fill.a = press * kPressMix;
            return fill;
        }
        if (hover > 0.001f) {
            Color fill = ColorSpace::OpaqueSurface(ResolveColor(ColorToken::HoverBackground));
            fill.a = hover * kHoverMix;
            return fill;
        }
        return Color::Transparent();
    }

    Color result = base;
    if (hover > 0.001f) {
        result = ColorSpace::LerpColor(
            result,
            ColorSpace::OpaqueSurface(ResolveColor(ColorToken::HoverBackground)),
            hover * kHoverMix);
    }
    if (press > 0.001f) {
        result = ColorSpace::LerpColor(
            result,
            ColorSpace::OpaqueSurface(ResolveColor(ColorToken::PressedBackground)),
            press * kPressMix);
    }
    return ColorSpace::OpaqueSurface(result);
}

Color ResolveInteractiveBackgroundImpl(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    if (selected) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }
    if (Clamp01(hoverAnim) < 0.001f && Clamp01(pressAnim) < 0.001f) {
        return Color::Transparent();
    }
    const Color base = ColorSpace::OpaqueSurface(ResolveColor(surfaceToken));
    return MixInteractiveSurfaceImpl(base, hoverAnim, pressAnim, false, false);
}

} // namespace

IKindUITheme& ResolveDefaultTheme() {
    return ThemeManager::Get().Theme();
}

Color ResolveColor(ColorToken token) {
    palette::ReloadGraphiteDarkPaletteIfChanged();
    Color resolved = ThemeManager::Get().Theme().ResolveColor(token);
    if (!IsCompositeColorToken(token)) {
        resolved = ColorSpace::OpaqueSurface(resolved);
    }
    if (UiColorDebug::IsEnabled()) {
        UiColorDebug::Get().TraceResolve(token, resolved);
    }
    return resolved;
}

float ResolveMetric(MetricToken token) {
    palette::ReloadGraphiteDarkPaletteIfChanged();
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

Color MixInteractiveSurface(
    Color base,
    float hoverAnim,
    float pressAnim,
    bool selected,
    bool disabled)
{
    palette::ReloadGraphiteDarkPaletteIfChanged();
    return MixInteractiveSurfaceImpl(base, hoverAnim, pressAnim, selected, disabled);
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
        if (hoverAnim > 0.001f || pressStrength > 0.001f) {
            accentColor = ColorSpace::LerpColor(
                accentColor,
                ResolveColor(ColorToken::AccentHover),
                Clamp01(std::max(hoverAnim, pressStrength)) * 0.25f);
        }
        return accentColor;
    }

    if (role == IconColorRole::Disabled) {
        return ResolveColor(ColorToken::IconDisabled);
    }

    Color base = ResolveColor(ColorToken::IconSecondary);
    if (role == IconColorRole::Primary) {
        base = ResolveColor(ColorToken::IconPrimary);
    }
    Color hover = ResolveColor(ColorToken::IconHover);
    Color pressed = ResolveColor(ColorToken::IconActive);
    Color result = ColorSpace::LerpColor(base, hover, Clamp01(hoverAnim) * 0.45f);
    result = ColorSpace::LerpColor(result, pressed, Clamp01(pressStrength) * 0.35f);
    return result;
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
