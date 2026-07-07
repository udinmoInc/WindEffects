#pragma once

#include "Scene/Export.h"
#include "Entity.h"
#if WE_HAS_VULKAN
#include <volk.h>
#endif
#include <vector>
#include <memory>

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
    void SetSelectedEntityIndex(int index) { m_SelectedEntityIndex = index; }

    SCENE_API void Update();

#if WE_HAS_VULKAN
    SCENE_API bool IsCameraBufferAssigned() const { return true; }
#endif

#if WE_HAS_VULKAN
    enum class DrawMode {
        Editor,
        Game
    };
    SCENE_API void Draw(VkCommandBuffer cmd, DrawMode drawMode = DrawMode::Editor) const;
#endif

private:
    std::uint64_t m_NextEntityId = 1;
    std::vector<Entity> m_Entities;
    int m_SelectedEntityIndex = -1;
};

} // namespace we::runtime::scene
