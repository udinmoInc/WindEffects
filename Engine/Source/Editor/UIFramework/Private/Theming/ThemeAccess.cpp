#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"

#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"

namespace WindEffects::Editor::UI {
namespace {

IThemeProvider& DefaultThemeProvider() {
    static GraphiteDarkTheme theme;
    return theme;
}

} // namespace

IThemeProvider& ResolveDefaultThemeProvider() {
    return DefaultThemeProvider();
}

Color ResolveThemeColor(ThemeToken token) {
    return DefaultThemeProvider().GetColor(token);
}

float ResolveThemeMetric(ThemeToken token) {
    return DefaultThemeProvider().GetMetric(token);
}

Margin ResolveThemePadding(ThemeToken token) {
    return DefaultThemeProvider().GetPadding(token);
}

Color ResolveThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected) {
    return DefaultThemeProvider().InteractiveBackground(hoverAnim, pressAnim, selected);
}

Color ResolveThemeTextForState(bool hovered, bool active) {
    return DefaultThemeProvider().TextForState(hovered, active);
}

Color ResolveThemeIconForState(bool hovered, bool active) {
    return DefaultThemeProvider().IconForState(hovered, active);
}

} // namespace WindEffects::Editor::UI
