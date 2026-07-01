#include "DefaultSceneBuilder.h"

#include "Environment/EnvironmentSystem.h"
#include "Core/Logger.hpp"

namespace we::runtime::world {

void DefaultSceneBuilder::CreateDefaultScene(World& world) {
    if (!world.IsCameraBufferAssigned() || !world.IsEmpty()) {
        return;
    }

    we::runtime::world::environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
}

} // namespace we::runtime::world
