#include "Framebuffer.hpp"
#include "../Core/Logger.hpp"
#include <stdexcept>
#include <array>
#include <iostream>

namespace HouseEngine {

Framebuffer::Framebuffer(const VulkanContext& context, uint32_t width, uint32_t height, VkRenderPass renderPass)
    : m_Context(context), m_Width(width), m_Height(height) {
    
    // Create Sampler (reused across resizes)
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_Context.GetDevice(), &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen framebuffer sampler!");
    }

    CreateResources(renderPass);
}

Framebuffer::~Framebuffer() {
    DestroyResources();
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Context.GetDevice(), m_Sampler, nullptr);
    }
}

void Framebuffer::Resize(uint32_t width, uint32_t height, VkRenderPass renderPass) {
    if (width == m_Width && height == m_Height && m_Framebuffer != VK_NULL_HANDLE) {
        return;
    }

    // Wait for GPU to finish using resources before destroying them
    vkDeviceWaitIdle(m_Context.GetDevice());

    m_Width = std::max(1u, width);
    m_Height = std::max(1u, height);

    DestroyResources();
    CreateResources(renderPass);
}

void Framebuffer::CreateResources(VkRenderPass renderPass) {
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    HE_INFO("Framebuffer: Creating color image & view...");
    m_Context.CreateImage(
        m_Width, m_Height,
        colorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_ColorImage, m_ColorImageMemory
    );

    m_ColorImageView = m_Context.CreateImageView(m_ColorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    HE_INFO("Framebuffer: Creating depth image & view...");
    m_Context.CreateImage(
        m_Width, m_Height,
        depthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_DepthImage, m_DepthImageMemory
    );

    m_DepthImageView = m_Context.CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    HE_INFO("Framebuffer: Transitioning color image layout...");
    m_Context.TransitionImageLayout(m_ColorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // 3. Create Framebuffer
    std::array<VkImageView, 2> attachments = {
        m_ColorImageView,
        m_DepthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = m_Width;
    framebufferInfo.height = m_Height;
    framebufferInfo.layers = 1;

    HE_INFO("Framebuffer: Creating Vulkan framebuffer...");
    if (vkCreateFramebuffer(m_Context.GetDevice(), &framebufferInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create offscreen Framebuffer!");
    }

    HE_INFO("Framebuffer: Resources successfully created.");
}

void Framebuffer::DestroyResources() {
    VkDevice device = m_Context.GetDevice();
    if (m_Framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
    if (m_ColorImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_ColorImageView, nullptr);
        m_ColorImageView = VK_NULL_HANDLE;
    }
    if (m_ColorImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_ColorImage, nullptr);
        m_ColorImage = VK_NULL_HANDLE;
    }
    if (m_ColorImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_ColorImageMemory, nullptr);
        m_ColorImageMemory = VK_NULL_HANDLE;
    }
    if (m_DepthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_DepthImageView, nullptr);
        m_DepthImageView = VK_NULL_HANDLE;
    }
    if (m_DepthImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_DepthImage, nullptr);
        m_DepthImage = VK_NULL_HANDLE;
    }
    if (m_DepthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_DepthImageMemory, nullptr);
        m_DepthImageMemory = VK_NULL_HANDLE;
    }
}

} // namespace HouseEngine
