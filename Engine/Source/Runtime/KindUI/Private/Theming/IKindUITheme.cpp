#include "KindUI/Theming/IKindUITheme.h"

namespace we::runtime::kindui {

Color IKindUITheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    if (pressAnim > 0.01f) {
        auto c = ResolveColor(ColorToken::PressedBackground);
        c.a *= pressAnim;
        return c;
    }
    if (hoverAnim > 0.01f) {
        return Color::Transparent().Lerp(ResolveColor(ColorToken::HoverBackground), hoverAnim);
    }
    if (selected) {
        return ResolveColor(ColorToken::SelectedBackground);
    }
    return Color::Transparent();
}

Color IKindUITheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconAccent);
    }
    Color base = ResolveColor(ColorToken::IconSecondary);
    if (hovered) {
        return Color::Lerp(base, ResolveColor(ColorToken::IconPrimary), 1.0f);
    }
    return base;
}

Color IKindUITheme::TextForState(bool hovered, bool active) const {
    return (active || hovered)
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
}

} // namespace we::runtime::kindui
