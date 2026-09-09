// ==============================================================================
// WindEffects — PlaceActors — PlaceActorsRegistration
// Public API surface for the PlaceActors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PlaceActors/Export.h"

namespace we::programs::editor {

/// Ensures Place Actors catalog + Actors-mode custom content are registered.
/// Safe to call multiple times (e.g. from module StartupModule).
PLACEACTORS_API void EnsurePlaceActorsRegistered();

} // namespace we::programs::editor
