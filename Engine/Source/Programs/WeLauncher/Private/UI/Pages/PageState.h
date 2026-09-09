// ==============================================================================
// WindEffects — WeLauncher — PageState
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/App/ViewHost.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollLayout.h"

#include <memory>

namespace we::programs::welauncher {

struct PageState {
    std::unique_ptr<we::runtime::kindui::ViewHost> viewHost;
    std::shared_ptr<we::runtime::kindui::Widget> root;
    std::shared_ptr<we::runtime::kindui::ScrollLayout> scroll;
    float scrollOffset = 0.0f;
    bool built = false;
    bool dirty = true;
};

} // namespace we::programs::welauncher
