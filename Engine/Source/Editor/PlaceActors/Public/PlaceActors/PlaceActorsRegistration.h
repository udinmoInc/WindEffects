#pragma once

#include "PlaceActors/Export.h"

namespace we::programs::editor {

/// Ensures Place Actors catalog + Actors-mode custom content are registered.
/// Safe to call multiple times (e.g. from module StartupModule).
PLACEACTORS_API void EnsurePlaceActorsRegistered();

} // namespace we::programs::editor
