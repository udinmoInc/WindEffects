// ==============================================================================
// WindEffects — WeLauncher — ProjectsTypes
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

namespace we::programs::welauncher {

enum class ProjectCardAction {
    Open,
    Clone,
    Rename,
    Delete,
    ShowInExplorer,
    Favorite,
    More,
    Regenerate
};

} // namespace we::programs::welauncher
