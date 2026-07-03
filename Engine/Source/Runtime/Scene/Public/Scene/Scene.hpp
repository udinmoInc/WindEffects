#pragma once

#include "Scene/Export.hpp"
#include "Entity.hpp"
#if WE_HAS_VULKAN
#include "Renderer/VulkanContext.hpp"
#include "Renderer/SceneRenderer.hpp"
#endif
#include <vector>
#include <memory>

namespace we::runtime::scene {

class Scene {
public:
#if WE_HAS_VULKAN
    SCENE_API Scene(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer);
#else
    SCENE_API Scene();
#endif
    SCENE_API ~Scene();

    // Prevent copying
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

#if WE_HAS_VULKAN
    SCENE_API void SetCameraBuffer(VkBuffer cameraBuffer);
    SCENE_API bool IsCameraBufferAssigned() const;
#endif

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
    enum class DrawMode {
        Editor,
        Game
    };
    SCENE_API void Draw(VkCommandBuffer cmd, DrawMode drawMode = DrawMode::Editor) const;
#endif

private:
#if WE_HAS_VULKAN
    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    std::shared_ptr<we::runtime::renderer::SceneRenderer> m_Renderer;
    VkBuffer m_CameraBuffer = VK_NULL_HANDLE;
#endif
    std::uint64_t m_NextEntityId = 1;
    std::vector<Entity> m_Entities;
    int m_SelectedEntityIndex = -1;
};

} // namespace we::runtime::scene
