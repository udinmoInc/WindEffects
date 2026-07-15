#pragma once

// WindEffects Scene SDK — entities, hierarchy, and selection.

#include "WindEffects/Platform.h"

#include "Scene/Scene.h"
#include "Scene/Entity.h"

#include <string_view>

namespace we::runtime::scene {

inline Entity* SelectEntity(Scene& scene, std::uint64_t entityId) {
    scene.SetSelectedEntityId(entityId);
    return scene.FindEntityById(entityId);
}

inline Entity* FindByName(Scene& scene, std::string_view name) {
    const int index = scene.FindEntityIndexByName(std::string(name));
    return index >= 0 ? &scene.GetEntities()[static_cast<size_t>(index)] : nullptr;
}

} // namespace we::runtime::scene
