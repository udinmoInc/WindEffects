#pragma once

#include "Terrain/Export.h"

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::terrain {

/// Registers TerrainCreateInfo / brush settings for Property Editor + Undo diffs.
TERRAIN_API void RegisterTerrainReflection(reflection::ITypeRegistry& registry);

} // namespace we::runtime::terrain
