#pragma once

#include "Scene/Export.h"
#include "Entity.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::ecs {
class Registry;
class SystemScheduler;
}

namespace we::runtime::scene {

class Scene {
public:
    SCENE_API Scene();
    SCENE_API ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    SCENE_API void CreateEntity(const std::string& name, EntityType type);
    SCENE_API bool HasEntityOfType(EntityType type) const;
    SCENE_API bool HasEntityNamed(const std::string& name) const;
    SCENE_API int FindEntityIndexById(std::uint64_t id) const;
    SCENE_API int FindEntityIndexByName(const std::string& name) const;
    SCENE_API Entity* FindEntityById(std::uint64_t id);
    SCENE_API const Entity* FindEntityById(std::uint64_t id) const;
    SCENE_API std::vector<int> FindChildIndices(std::uint64_t parentId) const;
    SCENE_API bool IsEmpty() const;
    SCENE_API void Clear();
    SCENE_API void DestroyEntity(size_t index);

    std::vector<Entity>& GetEntities() { return m_Entities; }
    const std::vector<Entity>& GetEntities() const { return m_Entities; }

    int GetSelectedEntityIndex() const { return m_SelectedEntityIndex; }
    SCENE_API void SetSelectedEntityIndex(int index);

    std::uint64_t GetSelectedEntityId() const { return m_SelectedEntityId; }
    SCENE_API void SetSelectedEntityId(std::uint64_t id);

    SCENE_API we::runtime::ecs::Registry& Registry();
    SCENE_API const we::runtime::ecs::Registry& Registry() const;
    SCENE_API we::runtime::ecs::SystemScheduler& Systems();

    SCENE_API void Update();
    SCENE_API void SyncLegacyToEcs();
    SCENE_API void SyncEcsToLegacy();

private:
    void AttachEcsComponents(Entity& entity, std::uint64_t ecsEntityId);

    std::unique_ptr<we::runtime::ecs::Registry> m_Registry;
    std::unique_ptr<we::runtime::ecs::SystemScheduler> m_Systems;
    std::vector<Entity> m_Entities;
    int m_SelectedEntityIndex = -1;
    std::uint64_t m_SelectedEntityId = 0;
};

} // namespace we::runtime::scene
