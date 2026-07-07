#include "DefaultSceneBuilder.h"

#include "EnvironmentSystem.h"
#include "Core/Logger.h"

namespace we::runtime::world {

void DefaultSceneBuilder::CreateDefaultScene(World& world) {
#if WE_HAS_VULKAN
    if (!world.IsCameraBufferAssigned() || !world.IsEmpty()) {
#else
    if (!world.IsEmpty()) {
#endif
        return;
    }

    we::runtime::world::environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
}

} // namespace we::runtime::world
