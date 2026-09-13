// ==============================================================================
// WindEffects — KindUI — IKindUITheme
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Theming/IKindUITheme.h"
#include "KindUI/Typography/TypographySystem.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::runtime::kindui {

TypographySpec IKindUITheme::ResolveTypography(const TypographyToken token) const {
    return TypographySystem::GetSpec(token);
}

Color IKindUITheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color IKindUITheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return ResolveColor(ColorToken::IconActive);
    }
    if (hovered) {
        return ResolveColor(ColorToken::IconHover);
    }
    return ResolveColor(ColorToken::IconSecondary);
}

Color IKindUITheme::TextForState(bool hovered, bool active) const {
    return (active || hovered)
        ? ResolveColor(ColorToken::TextPrimary)
        : ResolveColor(ColorToken::TextSecondary);
}

} // namespace we::runtime::kindui
 
