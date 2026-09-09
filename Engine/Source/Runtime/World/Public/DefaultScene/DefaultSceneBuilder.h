// ==============================================================================
// WindEffects — World — DefaultSceneBuilder
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"

#include "Scene/Scene.h"

namespace we::runtime::world {

/// Builds a default editor/gameplay starter scene on an existing Scene container.
/// Scene remains the ECS-backed container; World Runtime orchestrates higher-level lifetime.
class WORLD_API DefaultSceneBuilder {
public:
    static void CreateDefaultScene(scene::Scene& scene);
};

} // namespace we::runtime::world
