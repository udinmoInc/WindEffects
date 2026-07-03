#pragma once

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
    Scene(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer);
#else
    Scene();
#endif
    ~Scene();

    // Prevent copying
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

#if WE_HAS_VULKAN
    void SetCameraBuffer(VkBuffer cameraBuffer);
    bool IsCameraBufferAssigned() const;
#endif

    void CreateEntity(const std::string& name, EntityType type);
    bool HasEntityOfType(EntityType type) const;
    bool HasEntityNamed(const std::string& name) const;
    int FindEntityIndexById(std::uint64_t id) const;
    int FindEntityIndexByName(const std::string& name) const;
    Entity* FindEntityById(std::uint64_t id);
    const Entity* FindEntityById(std::uint64_t id) const;
    std::vector<int> FindChildIndices(std::uint64_t parentId) const;
    bool IsEmpty() const;
    void Clear();
    void DestroyEntity(size_t index);

    std::vector<Entity>& GetEntities() { return m_Entities; }
    const std::vector<Entity>& GetEntities() const { return m_Entities; }

    int GetSelectedEntityIndex() const { return m_SelectedEntityIndex; }
    void SetSelectedEntityIndex(int index) { m_SelectedEntityIndex = index; }

    void Update();

#if WE_HAS_VULKAN
    enum class DrawMode {
        Editor,
        Game
    };
    void Draw(VkCommandBuffer cmd, DrawMode drawMode = DrawMode::Editor) const;
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
