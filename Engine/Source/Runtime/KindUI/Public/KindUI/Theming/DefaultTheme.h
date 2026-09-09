// ==============================================================================
// WindEffects — KindUI — DefaultTheme
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Theming/GraphiteDarkTheme.h"

namespace we::runtime::kindui {

// Alias of GraphiteDark — single canonical palette; distinct theme id for framework defaults.
class KINDUI_API DefaultTheme final : public GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "KindUI.Default"; }
};

} // namespace we::runtime::kindui
