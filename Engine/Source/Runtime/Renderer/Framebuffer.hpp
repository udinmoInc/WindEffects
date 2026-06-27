#pragma once

#include "VulkanContext.hpp"
#include <volk.h>

namespace HouseEngine {

class Framebuffer {
public:
    Framebuffer(const VulkanContext& context, uint32_t width, uint32_t height, VkRenderPass renderPass);
    ~Framebuffer();

    // Prevent copying
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Resize(uint32_t width, uint32_t height, VkRenderPass renderPass);

    VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
    VkImageView GetColorImageView() const { return m_ColorImageView; }
    VkImageView GetDepthImageView() const { return m_DepthImageView; }
    VkSampler GetSampler() const { return m_Sampler; }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void CreateResources(VkRenderPass renderPass);
    void DestroyResources();

    const VulkanContext& m_Context;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;

    VkImage m_ColorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_ColorImageMemory = VK_NULL_HANDLE;
    VkImageView m_ColorImageView = VK_NULL_HANDLE;

    VkImage m_DepthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DepthImageMemory = VK_NULL_HANDLE;
    VkImageView m_DepthImageView = VK_NULL_HANDLE;

    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
};

} // namespace HouseEngine
