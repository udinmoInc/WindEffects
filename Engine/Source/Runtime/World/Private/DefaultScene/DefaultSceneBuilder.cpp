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
