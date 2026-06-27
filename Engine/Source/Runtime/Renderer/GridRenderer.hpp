#pragma once

#include "VulkanContext.hpp"
#include <volk.h>
#include <memory>

namespace HouseEngine {

class GridRenderer {
public:
    GridRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    ~GridRenderer();

    // Prevent copying
    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(const GridRenderer&) = delete;

    void Draw(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;

private:
    void CreatePipeline(VkRenderPass renderPass);

    std::shared_ptr<VulkanContext> m_Context;
    VkDescriptorSetLayout m_CameraDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
};

} // namespace HouseEngine
