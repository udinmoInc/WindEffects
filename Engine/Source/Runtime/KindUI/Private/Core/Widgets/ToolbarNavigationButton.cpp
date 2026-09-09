// ==============================================================================
// WindEffects — KindUI — ToolbarNavigationButton
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Widgets/ToolbarNavigationButton.h"

namespace we::runtime::kindui {

void ToolbarNavigationButton::SetOnClicked(std::function<void()> callback) {
    ToolbarGlyphButton::SetOnClicked(std::move(callback));
}

void ToolbarNavigationButton::SetEnabled(bool enabled) {
    Widget::SetEnabled(enabled);
}

void ToolbarNavigationButton::SetSelected(bool selected) {
    ToolbarGlyphButton::SetSelected(selected);
}

} // namespace we::runtime::kindui
 
