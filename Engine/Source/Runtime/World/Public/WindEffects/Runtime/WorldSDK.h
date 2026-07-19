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
