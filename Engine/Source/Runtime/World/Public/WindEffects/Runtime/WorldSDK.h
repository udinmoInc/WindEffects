// ==============================================================================
// WindEffects — World — WorldSDK
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects World SDK — World Runtime + environment + default scene setup.

#include "WindEffects/Platform.h"

#include "World/World.h"
#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentSettings.h"
#include "DefaultScene/DefaultSceneBuilder.h"
#include "DefaultScene/DefaultSceneSettings.h"
#include "Scene/Scene.h"

#include <memory>

namespace we::runtime::world {

inline void BindEnvironmentToScene(
    environment::EnvironmentSystem& environment,
    const std::shared_ptr<scene::Scene>& scene) {
    environment.BindScene(scene);
}

} // namespace we::runtime::world
