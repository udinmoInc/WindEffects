#include "KindUI/Theming/ThemeColors.h"

#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>


namespace we::runtime::kindui {

ThemeColors ThemeColors::Resolve(const IKindUITheme& theme) {
    ThemeColors colors{};
    colors.background = theme.ResolveColor(ColorToken::WorkspaceBackground);
    colors.panel = theme.ResolveColor(ColorToken::PanelBackground);
    colors.border = theme.ResolveColor(ColorToken::BorderDefault);
    colors.hover = theme.ResolveColor(ColorToken::HoverBackground);
    colors.pressed = theme.ResolveColor(ColorToken::PressedBackground);
    colors.selection = theme.ResolveColor(ColorToken::SelectedBackground);

    colors.textPrimary = theme.ResolveColor(ColorToken::TextPrimary);
    colors.textSecondary = theme.ResolveColor(ColorToken::TextSecondary);
    colors.textDisabled = theme.ResolveColor(ColorToken::TextDisabled);

    colors.iconPrimary = theme.ResolveColor(ColorToken::IconPrimary);
    colors.iconSecondary = theme.ResolveColor(ColorToken::IconSecondary);
    colors.iconAccent = theme.ResolveColor(ColorToken::IconAccent);
    colors.iconDisabled = theme.ResolveColor(ColorToken::IconDisabled);
    return colors;
}

ThemeColors ThemeColors::Resolve() {
    return Resolve(ResolveDefaultTheme());
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
    const float emphasis = std::max(hoverAnim, pressStrength);
    if (emphasis > 0.01f) {
        Color bright = ResolveColor(ColorToken::IconPrimary);
        bright = Color::Lerp(bright, ResolveColor(ColorToken::IconHover), emphasis);
        base = Color::Lerp(base, bright, emphasis);
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
        IconColorRole::Secondary,
        hovered ? 1.0f : 0.0f,
        0.0f,
        accent);
}

} // namespace we::runtime::kindui
