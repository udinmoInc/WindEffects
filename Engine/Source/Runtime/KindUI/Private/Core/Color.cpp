// ==============================================================================
// WindEffects — KindUI — Color
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/Types.h"
#include "KindUI/Theming/PaletteRuntime.h"

namespace we::runtime::kindui {

Color Color::White() {
    return palette::GraphiteDarkLive().White;
}

Color Color::Black() {
    return palette::GraphiteDarkLive().Black;
}

} // namespace we::runtime::kindui
 
