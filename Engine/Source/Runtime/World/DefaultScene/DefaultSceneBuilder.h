#pragma once

#include "Scene/Scene.hpp"

namespace we::runtime::world {

using World = we::runtime::scene::Scene;

class DefaultSceneBuilder {
public:
    static void CreateDefaultScene(World& world);
};

} // namespace we::runtime::world
