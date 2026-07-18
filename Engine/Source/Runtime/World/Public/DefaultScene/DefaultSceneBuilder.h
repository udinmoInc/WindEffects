#pragma once

#include "World/Export.h"

#include "Scene/Scene.h"

namespace we::runtime::world {

using World = we::runtime::scene::Scene;

class WORLD_API DefaultSceneBuilder {
public:
    static void CreateDefaultScene(World& world);
};

} // namespace we::runtime::world
