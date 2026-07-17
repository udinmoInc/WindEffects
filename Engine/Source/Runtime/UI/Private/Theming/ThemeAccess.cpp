#include "WindEffects/Runtime/UI/Theming/ThemeAccess.h"

#include "WindEffects/Runtime/UI/Theming/ThemeManager.h"

namespace WindEffects::Editor::UI {

IThemeProvider& ResolveDefaultThemeProvider() {
    return ThemeManager::Get().Provider();
}

Color ResolveThemeColor(ThemeToken token) {
    return ThemeManager::Get().Provider().GetColor(token);
}

float ResolveThemeMetric(ThemeToken token) {
    return ThemeManager::Get().Provider().GetMetric(token);
}

Margin ResolveThemePadding(ThemeToken token) {
    return ThemeManager::Get().Provider().GetPadding(token);
}

Color ResolveThemeInteractiveBackground(float hoverAnim, float pressAnim, bool selected) {
    return ThemeManager::Get().Provider().InteractiveBackground(hoverAnim, pressAnim, selected);
}

Color ResolveThemeTextForState(bool hovered, bool active) {
    return ThemeManager::Get().Provider().TextForState(hovered, active);
}

Color ResolveThemeIconForState(bool hovered, bool active) {
    return ThemeManager::Get().Provider().IconForState(hovered, active);
}

} // namespace WindEffects::Editor::UI
