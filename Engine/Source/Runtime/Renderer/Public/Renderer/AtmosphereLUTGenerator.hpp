#pragma once

#if WE_HAS_VULKAN
#include "Renderer/VulkanContext.hpp"
#include "Renderer/SceneEnvironmentUniform.hpp"
#include <volk.h>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::renderer {

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

struct AtmosphereLUTDimensions {
    uint32_t transmittanceW = 256;
    uint32_t transmittanceH = 64;
    uint32_t multiScatterSize = 32;
    uint32_t skyViewW = 192;
    uint32_t skyViewH = 96;
    uint32_t aerialSize = 32;
};

class AtmosphereLUTGenerator {
public:
    explicit AtmosphereLUTGenerator(const std::shared_ptr<VulkanContext>& context);
    ~AtmosphereLUTGenerator();

    AtmosphereLUTGenerator(const AtmosphereLUTGenerator&) = delete;
    AtmosphereLUTGenerator& operator=(const AtmosphereLUTGenerator&) = delete;

    bool EnsureGenerated(const SceneEnvironmentUniform& environment);
    void Invalidate() { m_Dirty = true; m_Ready = false; }

    bool IsReady() const { return m_Ready && m_HasGenerated; }
    const std::string& GetLastError() const { return m_LastError; }
    const AtmosphereLUTDimensions& GetDimensions() const { return m_Dimensions; }

    VkDescriptorSetLayout GetSampleLayout() const { return m_SampleDescLayout; }
    VkDescriptorSet GetSampleSet() const { return m_SampleDescSet; }
    const AtmosphereLUTImages& GetImages() const { return m_Images; }

    bool ValidateResources(std::string* failureReason = nullptr) const;

private:
    void CreateResources();
    void DestroyResources();
    void CreateDescriptorResources();
    void DestroyDescriptorResources();
    void CreateLUTImage(uint32_t width, uint32_t height, VkImage& image, VkDeviceMemory& memory, VkImageView& view);
    bool UploadLUT(VkImage image, uint32_t width, uint32_t height, const std::vector<float>& rgba, const char* lutName);
    bool GenerateCPU(const SceneEnvironmentUniform& environment);
    void TransitionLUTForUpload(VkImage image);
    bool UpdateSampleDescriptors();
    bool ValidateLUTData(const char* lutName, const std::vector<float>& rgba, uint32_t width, uint32_t height);

    std::shared_ptr<VulkanContext> m_Context;
    AtmosphereLUTImages m_Images{};
    AtmosphereLUTDimensions m_Dimensions{};

    VkDescriptorSetLayout m_SampleDescLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_SampleDescSet = VK_NULL_HANDLE;

    bool m_Dirty = true;
    bool m_Ready = false;
    bool m_HasGenerated = false;
    SceneEnvironmentUniform m_LastGeneratedEnvironment{};
    std::string m_LastError;
};

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
