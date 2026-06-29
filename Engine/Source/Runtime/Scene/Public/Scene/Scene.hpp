#pragma once

#include "Entity.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/SceneRenderer.hpp"
#include <vector>
#include <memory>

namespace we::runtime::scene {

class Scene {
public:
    Scene(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, const std::shared_ptr<we::runtime::renderer::SceneRenderer>& renderer);
    ~Scene();

    // Prevent copying
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    void InitializeDefaultScene(VkBuffer cameraBuffer);

    void CreateEntity(const std::string& name, EntityType type, VkBuffer cameraBuffer);
    void DestroyEntity(size_t index);

    std::vector<Entity>& GetEntities() { return m_Entities; }
    const std::vector<Entity>& GetEntities() const { return m_Entities; }

    int GetSelectedEntityIndex() const { return m_SelectedEntityIndex; }
    void SetSelectedEntityIndex(int index) { m_SelectedEntityIndex = index; }

    void Update();

    bool HasGroundCoverage() const;
    bool ShouldDrawEditorGrid() const { return !HasGroundCoverage(); }
    void Draw(VkCommandBuffer cmd) const;

private:
    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    std::shared_ptr<we::runtime::renderer::SceneRenderer> m_Renderer;
    std::vector<Entity> m_Entities;
    int m_SelectedEntityIndex = -1;
};

} // namespace we::runtime::scene
