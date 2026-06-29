#pragma once

#include "Renderer/VulkanContext.hpp"
#include <volk.h>
#include <memory>

namespace we::runtime::renderer {

class GridRenderer {
public:
    GridRenderer(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass, VkDescriptorSetLayout cameraDescLayout);
    ~GridRenderer();

    GridRenderer(const GridRenderer&) = delete;
    GridRenderer& operator=(const GridRenderer&) = delete;

    void Draw(VkCommandBuffer cmd, VkDescriptorSet cameraDescSet) const;

    void SetGridFadeDistance(float distance);
    void SetGridLodIntensity(float intensity);

    // Selects grid LOD once per frame from camera distance / zoom, then uploads uniforms.
    void UpdateFromCamera(float lodDistance, float fovDegrees, float viewportHeight, float projYScale);

private:
    void CreatePipeline(VkRenderPass renderPass);
    void UpdateGridSettingsBufferIfDirty();

    struct GridSettings {
        float cellSizeA = 1.0f;
        float cellSizeB = 1.0f;
        float lodBlend = 0.0f;
        float hdrScale = 1.0f;
        float fadeDistance = 8000.0f;
        float lodIntensity = 1.0f;
        float originSnap = 1.0f;
        float thicknessScale = 1.0f;
    };

    std::shared_ptr<VulkanContext> m_Context;
    VkDescriptorSetLayout m_CameraDescLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_GridDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_GridDescSet = VK_NULL_HANDLE;
    VkBuffer m_GridSettingsBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_GridSettingsMemory = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    GridSettings m_GridSettings{};
    bool m_GridSettingsDirty = true;
};

} // namespace we::runtime::renderer
