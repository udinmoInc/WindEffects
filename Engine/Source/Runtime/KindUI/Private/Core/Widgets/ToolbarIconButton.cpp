// ==============================================================================
// WindEffects — KindUI — ToolbarIconButton
// UI widget used by the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Widgets/ToolbarIconButton.h"

namespace we::runtime::kindui {

void ToolbarIconButton::SetOnClicked(std::function<void()> callback) {
    ToolbarGlyphButton::SetOnClicked(std::move(callback));
}

} // namespace we::runtime::kindui
