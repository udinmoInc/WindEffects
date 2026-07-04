#pragma once

#if WE_HAS_VULKAN
#include "Renderer/VulkanContext.hpp"
#include <volk.h>
#include <memory>
#include <vector>

namespace we::runtime::renderer {

struct SceneEnvironmentUniform;

struct AtmosphereLUTImages {
    VkImage transmittance = VK_NULL_HANDLE;
    VkDeviceMemory transmittanceMemory = VK_NULL_HANDLE;
    VkImageView transmittanceView = VK_NULL_HANDLE;

    VkImage multiScatter = VK_NULL_HANDLE;
    VkDeviceMemory multiScatterMemory = VK_NULL_HANDLE;
    VkImageView multiScatterView = VK_NULL_HANDLE;

    VkImage skyView = VK_NULL_HANDLE;
    VkDeviceMemory skyViewMemory = VK_NULL_HANDLE;
    VkImageView skyViewView = VK_NULL_HANDLE;

    VkImage aerial = VK_NULL_HANDLE;
    VkDeviceMemory aerialMemory = VK_NULL_HANDLE;
    VkImageView aerialView = VK_NULL_HANDLE;

    VkSampler sampler = VK_NULL_HANDLE;
};

class AtmosphereLUTGenerator {
public:
    explicit AtmosphereLUTGenerator(const std::shared_ptr<VulkanContext>& context);
    ~AtmosphereLUTGenerator();

    AtmosphereLUTGenerator(const AtmosphereLUTGenerator&) = delete;
    AtmosphereLUTGenerator& operator=(const AtmosphereLUTGenerator&) = delete;

    void EnsureGenerated(const SceneEnvironmentUniform& environment);
    void Invalidate() { m_Dirty = true; }

    VkDescriptorSetLayout GetSampleLayout() const { return m_SampleDescLayout; }
    VkDescriptorSet GetSampleSet() const { return m_SampleDescSet; }
    const AtmosphereLUTImages& GetImages() const { return m_Images; }

private:
    void CreateResources();
    void DestroyResources();
    void CreateDescriptorResources();
    void DestroyDescriptorResources();
    void CreateLUTImage(uint32_t width, uint32_t height, VkImage& image, VkDeviceMemory& memory, VkImageView& view);
    void UploadLUT(VkImage image, uint32_t width, uint32_t height, const std::vector<float>& rgba);
    void GenerateCPU(const SceneEnvironmentUniform& environment);

    std::shared_ptr<VulkanContext> m_Context;
    AtmosphereLUTImages m_Images{};

    VkDescriptorSetLayout m_SampleDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SampleDescSet = VK_NULL_HANDLE;

    bool m_Dirty = true;
    SceneEnvironmentUniform m_LastEnvironment{};
};

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
