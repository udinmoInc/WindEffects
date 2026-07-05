#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/RendererConfig.hpp"
#include <volk.h>
#include <memory>
#endif

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class PostProcessStack {
public:
    RENDERER_API explicit PostProcessStack(const std::shared_ptr<VulkanContext>& context);
    RENDERER_API ~PostProcessStack();

    PostProcessStack(const PostProcessStack&) = delete;
    PostProcessStack& operator=(const PostProcessStack&) = delete;

    RENDERER_API void Resize(uint32_t width, uint32_t height);
    RENDERER_API void Apply(
        VkCommandBuffer cmd,
        VkDescriptorSet environmentDescSet,
        VkImage sceneImage,
        VkImageView sceneImageView,
        uint32_t width,
        uint32_t height) const;

    RENDERER_API bool IsReady() const;

private:
    struct BloomImage {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    void DestroyResources();
    void CreateResources(uint32_t width, uint32_t height);
    void CreatePipelines();
    void DestroyPipelines();
    VkPipeline CreateComputePipeline(const char* shaderName, VkPipelineLayout layout) const;

    std::shared_ptr<VulkanContext> m_Context;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    uint32_t m_BloomWidth = 0;
    uint32_t m_BloomHeight = 0;
    uint32_t m_TileWidth = 0;
    uint32_t m_TileHeight = 0;

    BloomImage m_BloomA{};
    BloomImage m_BloomB{};
    BloomImage m_LuminanceTiles{};
    BloomImage m_LuminanceAvg{};

    VkSampler m_LinearSampler = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_PostDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_PostDescSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_SceneSampleLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_StorageLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_EnvDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SceneSampleSet = VK_NULL_HANDLE;
    VkDescriptorSet m_StorageSetA = VK_NULL_HANDLE;
    VkDescriptorSet m_StorageSetB = VK_NULL_HANDLE;
    VkDescriptorSet m_StorageSampleSet = VK_NULL_HANDLE;
    VkPipelineLayout m_LuminanceReduceLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_LuminanceAvgLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_BloomPrefilterLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_BloomBlurLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_CompositeLayout = VK_NULL_HANDLE;

    VkPipeline m_LuminanceReducePipeline = VK_NULL_HANDLE;
    VkPipeline m_LuminanceAvgPipeline = VK_NULL_HANDLE;
    VkPipeline m_BloomPrefilterPipeline = VK_NULL_HANDLE;
    VkPipeline m_BloomBlurPipeline = VK_NULL_HANDLE;
    VkPipeline m_CompositePipeline = VK_NULL_HANDLE;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
