#pragma once

#include <memory>
#include <volk.h>

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::runtime::renderer {
class VulkanContext;
}

namespace we::editor::grid {

// Editor-only infinite ground grid renderer. All grid logic lives here; callers invoke Render() only.
class EditorGridRenderer {
public:
    static EditorGridRenderer& Get();

    void Initialize(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context,
                    VkRenderPass renderPass,
                    VkDescriptorSetLayout cameraDescLayout);
    void Shutdown();

    void Render(VkCommandBuffer commandBuffer,
                VkDescriptorSet cameraDescriptorSet,
                const we::runtime::engine::EditorCamera& camera) const;

    bool IsInitialized() const { return m_Initialized; }

private:
    EditorGridRenderer() = default;

    void CreateResources(VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    void DestroyResources();
    void CreatePipeline(VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    void UploadUniforms(const we::runtime::engine::EditorCamera& camera) const;

    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    VkDescriptorSetLayout m_GridDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_GridDescSet = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkBuffer m_UniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_UniformMemory = VK_NULL_HANDLE;

    bool m_Initialized = false;
};

} // namespace we::editor::grid
