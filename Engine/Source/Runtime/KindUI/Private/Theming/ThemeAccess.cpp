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
        return true;
    default:
        return false;
    }
}

Color ResolveInteractiveBackgroundImpl(
    float hoverAnim,
    float pressAnim,
    bool selected,
    ColorToken surfaceToken)
{
    (void)surfaceToken;
    if (selected) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::SelectedBackground));
    }
    if (pressAnim >= 0.5f) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::PressedBackground));
    }
    if (hoverAnim >= 0.5f) {
        return ColorSpace::OpaqueSurface(ResolveColor(ColorToken::HoverBackground));
    }
    return Color::Transparent();
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
        if (hoverAnim >= 0.5f || pressStrength >= 0.5f) {
            accentColor = ResolveColor(ColorToken::AccentHover);
        }
        return accentColor;
    }

    if (role == IconColorRole::Disabled) {
        return ResolveColor(ColorToken::IconDisabled);
    }

    Color base = ResolveColor(ColorToken::IconSecondary);
    if (pressStrength >= 0.5f) {
        base = ResolveColor(ColorToken::IconActive);
    } else if (hoverAnim >= 0.5f) {
        base = ResolveColor(ColorToken::IconHover);
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
