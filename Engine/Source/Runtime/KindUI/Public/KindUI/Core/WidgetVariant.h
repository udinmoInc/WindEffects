// ==============================================================================
// WindEffects — KindUI — WidgetVariant
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>

namespace we::runtime::kindui {

// Semantic widget variants. Applications choose variants; themes decide appearance.
enum class WidgetVariant : uint32_t {
    Default,
    Primary,
    Secondary,
    Toolbar,
    Flat,
    Ghost,
    Link,
    Outline,
    Success,
    Warning,
    Danger,
    Accent,
};

} // namespace we::runtime::kindui
