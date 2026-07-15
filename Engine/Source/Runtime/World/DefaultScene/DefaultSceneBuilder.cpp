#include "DefaultSceneBuilder.h"

#include "EnvironmentSystem.h"
#include "Core/Logger.h"

namespace we::runtime::world {

void DefaultSceneBuilder::CreateDefaultScene(World& world) {
    if (!world.IsEmpty()) {
        return;
    }

    we::runtime::world::environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
}

} // namespace we::runtime::world
