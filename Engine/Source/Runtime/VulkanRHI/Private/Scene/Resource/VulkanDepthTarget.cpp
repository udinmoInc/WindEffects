#include "Resource/VulkanDepthTarget.h"

#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"

namespace we::runtime::renderer {

VulkanDepthTarget::~VulkanDepthTarget() {
    Shutdown();
}

void VulkanDepthTarget::Init(DeviceContext* deviceContext, ResourceManager* resourceManager,
                             const uint32_t width, const uint32_t height, const VkFormat format) {
    Shutdown();
    m_DeviceContext = deviceContext;
    m_ResourceManager = resourceManager;
    m_Width = width;
    m_Height = height;
    m_Format = format;
    if (m_DeviceContext && m_ResourceManager && m_Width > 0 && m_Height > 0 && m_Format != VK_FORMAT_UNDEFINED) {
        CreateResources();
    }
}

void VulkanDepthTarget::Shutdown() {
    if (m_ResourceManager) {
        if (m_ImageView != VK_NULL_HANDLE) {
            m_ResourceManager->DestroyImageView(m_ImageView);
            m_ImageView = VK_NULL_HANDLE;
        }
        if (m_Image != VK_NULL_HANDLE) {
            m_ResourceManager->DestroyImage(m_Image, m_Memory);
            m_Image = VK_NULL_HANDLE;
            m_Memory = VK_NULL_HANDLE;
        }
    }
    m_DeviceContext = nullptr;
    m_ResourceManager = nullptr;
    m_Width = 0;
    m_Height = 0;
    m_Format = VK_FORMAT_UNDEFINED;
}

void VulkanDepthTarget::Resize(const uint32_t width, const uint32_t height) {
    if (width == m_Width && height == m_Height) {
        return;
    }
    auto* deviceContext = m_DeviceContext;
    auto* resourceManager = m_ResourceManager;
    const VkFormat format = m_Format;
    Shutdown();
    Init(deviceContext, resourceManager, width, height, format);
}

void VulkanDepthTarget::CreateResources() {
    if (!m_ResourceManager) {
        return;
    }
    m_ResourceManager->CreateImage(
        m_Width,
        m_Height,
        m_Format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory);
    m_ImageView = m_ResourceManager->CreateImageView(m_Image, m_Format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

} // namespace we::runtime::renderer
