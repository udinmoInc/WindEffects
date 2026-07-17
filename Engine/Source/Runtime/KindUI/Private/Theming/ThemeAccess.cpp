#include "KindUI/Theming/ThemeAccess.h"

#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Tokens/TypographySpec.h"


namespace we::runtime::kindui {

IKindUITheme& ResolveDefaultTheme() {
    return ThemeManager::Get().Theme();
}

Color ResolveColor(ColorToken token) {
    return ThemeManager::Get().Theme().ResolveColor(token);
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

Color ResolveInteractiveBackground(float hoverAnim, float pressAnim, bool selected) {
    return ThemeManager::Get().Theme().InteractiveBackground(hoverAnim, pressAnim, selected);
}

Color ResolveTextForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().TextForState(hovered, active);
}

Color ResolveIconForState(bool hovered, bool active) {
    return ThemeManager::Get().Theme().IconForState(hovered, active);
}

} // namespace we::runtime::kindui
