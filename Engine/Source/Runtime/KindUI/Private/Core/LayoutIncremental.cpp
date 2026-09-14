// ==============================================================================
// WindEffects — KindUI — LayoutIncremental
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/LayoutIncremental.h"

namespace we::runtime::kindui {

LayoutIncrementalStats& LayoutIncrementalStats::Current() {
    static LayoutIncrementalStats s;
    return s;
}

void LayoutIncrementalStats::ResetCurrent() {
    Current().Reset();
}

} // namespace we::runtime::kindui
