#pragma once

#include "Renderer/Export.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"

namespace we::runtime::renderer {

// Returns true when CPU atmosphere LUTs must be rebuilt for the new environment.
RENDERER_API bool AtmosphereLUTInputsChanged(
    const SceneEnvironmentUniform& previous,
    const SceneEnvironmentUniform& next);

} // namespace we::runtime::renderer
