// ==============================================================================
// WindEffects — KindUI — IKindUITheme
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Theme/IKindUITheme.h"
#include "KindUI/Theme/TypographySystem.h"
#include "KindUI/Theme/ThemeAccess.h"

namespace we::runtime::kindui {

TypographySpec IKindUITheme::ResolveTypography(const TypographyToken token) const {
    return TypographySystem::GetSpec(token);
}

Color IKindUITheme::InteractiveBackground(float hoverAnim, float pressAnim, bool selected) const {
    // Single blender ownership: ThemeAccess::ResolveInteractiveBackground.
    return ResolveInteractiveBackground(hoverAnim, pressAnim, selected, ColorToken::PanelBackground);
}

Color IKindUITheme::IconForState(bool hovered, bool active) const {
    if (active) {
        return this->ResolveColor(ColorToken::IconActive);
    }
    if (hovered) {
        return this->ResolveColor(ColorToken::IconHover);
    }
    return this->ResolveColor(ColorToken::IconSecondary);
}

Color IKindUITheme::TextForState(bool hovered, bool active) const {
    return (active || hovered)
        ? this->ResolveColor(ColorToken::TextPrimary)
        : this->ResolveColor(ColorToken::TextSecondary);
}

} // namespace we::runtime::kindui
