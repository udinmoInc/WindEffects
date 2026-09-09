// ==============================================================================
// WindEffects — EditorShell — EditorTheme
// Internal implementation for the EditorShell module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "WindEffects/Editor/UI/Theming/EditorTheme.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::services {

we::runtime::kindui::Color EditorTheme::ResolveColor(we::runtime::kindui::ColorToken token) const {
    // Editor uses the GraphiteDark palette directly.
    return GraphiteDarkTheme::ResolveColor(token);
}

} // namespace we::editor::services
