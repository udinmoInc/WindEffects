#include "WindEffects/Editor/UI/Theming/ThemeColors.h"

#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"

#include <algorithm>

namespace WindEffects::Editor::UI {

ThemeColors ThemeColors::Resolve(const IThemeProvider& theme) {
    ThemeColors colors{};
    colors.background = theme.GetColor(ThemeToken::WorkspaceBackground);
    colors.panel = theme.GetColor(ThemeToken::PanelBackground);
    colors.border = theme.GetColor(ThemeToken::BorderDefault);
    colors.hover = theme.GetColor(ThemeToken::HoverBackground);
    colors.pressed = theme.GetColor(ThemeToken::PressedBackground);
    colors.selection = theme.GetColor(ThemeToken::SelectedBackground);

    colors.textPrimary = theme.GetColor(ThemeToken::TextPrimary);
    colors.textSecondary = theme.GetColor(ThemeToken::TextSecondary);
    colors.textDisabled = theme.GetColor(ThemeToken::TextDisabled);

    colors.iconPrimary = theme.GetColor(ThemeToken::IconPrimary);
    colors.iconSecondary = theme.GetColor(ThemeToken::IconSecondary);
    colors.iconAccent = theme.GetColor(ThemeToken::IconAccent);
    colors.iconDisabled = theme.GetColor(ThemeToken::IconDisabled);
    return colors;
}

ThemeColors ThemeColors::Resolve() {
    return Resolve(ResolveDefaultThemeProvider());
}

Color ResolveIconColor(
    IconColorRole role,
    float hoverAnim,
    float pressStrength,
    bool accent)
{
    if (accent || role == IconColorRole::Accent) {
        Color accentColor = ResolveThemeColor(ThemeToken::IconAccent);
        if (hoverAnim > 0.01f || pressStrength > 0.01f) {
            const Color hover = ResolveThemeColor(ThemeToken::AccentHover);
            accentColor = Color::Lerp(accentColor, hover, std::max(hoverAnim, pressStrength));
        }
        return accentColor;
    }

    if (role == IconColorRole::Disabled) {
        return ResolveThemeColor(ThemeToken::IconDisabled);
    }

    Color base = role == IconColorRole::Secondary
        ? ResolveThemeColor(ThemeToken::IconSecondary)
        : ResolveThemeColor(ThemeToken::IconPrimary);

    const float emphasis = std::max(hoverAnim, pressStrength);
    if (emphasis > 0.01f) {
        const Color hover = ResolveThemeColor(ThemeToken::IconHover);
        base = Color::Lerp(base, hover, emphasis);
    }
    return base;
}

Color ResolveIconColorForState(bool hovered, bool accent, bool disabled, bool secondary) {
    if (disabled) {
        return ResolveIconColor(IconColorRole::Disabled);
    }
    if (accent) {
        return ResolveIconColor(IconColorRole::Accent, hovered ? 1.0f : 0.0f, 0.0f, true);
    }
    return ResolveIconColor(
        secondary ? IconColorRole::Secondary : IconColorRole::Primary,
        hovered ? 1.0f : 0.0f);
}

} // namespace WindEffects::Editor::UI
