#pragma once

#include <volk.h>
#include <cstdint>

namespace we::runtime::renderer {
    class ResourceManager;
}

namespace we::editor::rendering {

struct EditorCompositorVertex {
    float position[2];
    float uv[2];
    float color[4];
    float sdfRect[4];
    float sdfParams[4];
};

class EditorCompositor {
public:
    EditorCompositor();
    ~EditorCompositor();

    bool Init(VkPhysicalDevice physicalDevice,
              VkDevice logicalDevice,
              we::runtime::renderer::ResourceManager* resourceManager,
              VkFormat swapchainFormat,
              uint32_t maxFramesInFlight);

    void Shutdown();

    void CompositeViewport(VkCommandBuffer cmd,
                            uint32_t frameIndex,
                            VkImageView swapchainView,
                            VkExtent2D swapchainExtent,
                            VkRect2D viewportDestRect,
                            VkImageView viewportColorImageView,
                            bool drawCrosshair);

private:
    void DestroyPipelineAndLayout();
    bool CreatePipeline();
    bool CreateDescriptorResources();
    bool CreateCompositorGeometry();
    void UpdateVertexBuffer(uint32_t frameIndex, VkRect2D viewportDestRect, bool drawCrosshair);
    void UpdateViewportDescriptorSet(uint32_t frameIndex, VkImageView viewportColorImageView);

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    we::runtime::renderer::ResourceManager* m_ResourceManager = nullptr;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    uint32_t m_MaxFramesInFlight = 0;

    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_TextureDescriptorSets[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};

    VkSampler m_Sampler = VK_NULL_HANDLE;

    struct FrameGeometry {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    };
    FrameGeometry m_FrameGeometry[2];
};

} // namespace we::editor::rendering

