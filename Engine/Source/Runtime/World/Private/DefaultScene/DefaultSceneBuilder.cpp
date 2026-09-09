// ==============================================================================
// WindEffects — World — DefaultSceneBuilder
// Internal implementation for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "DefaultScene/DefaultSceneBuilder.h"

#include "Environment/EnvironmentSystem.h"

namespace we::runtime::world {

void DefaultSceneBuilder::CreateDefaultScene(scene::Scene& scene) {
    if (!scene.IsEmpty()) {
        return;
    }

    environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
}

} // namespace we::runtime::world
