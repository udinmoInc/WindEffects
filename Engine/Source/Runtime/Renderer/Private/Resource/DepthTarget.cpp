#include "Resource/DepthTarget.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Resource/ResourceManager.h"

#if WE_HAS_VULKAN

namespace we::runtime::renderer {

DepthTarget::~DepthTarget() {
    Shutdown();
}

void DepthTarget::Init(const DepthTargetConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "DepthTarget", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "DepthTarget", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "DepthTarget", "ResourceManager is null.");
    WE_VALIDATE_INIT(config.width > 0 && config.height > 0, "DepthTarget", "Invalid dimensions.");

    m_DeviceContext = config.deviceContext;
    m_ResourceManager = config.resourceManager;
    m_Width = config.width;
    m_Height = config.height;

    CreateResources();
    m_Initialized = true;
}

void DepthTarget::Shutdown() {
    if (!m_Initialized) return;

    VkDevice device = m_DeviceContext->GetDevice();

    if (m_ImageView) {
        vkDestroyImageView(device, m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
    if (m_Image) {
        vkDestroyImage(device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }
    if (m_Memory) {
        vkFreeMemory(device, m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }

    m_Initialized = false;
}

void DepthTarget::Resize(uint32_t width, uint32_t height) {
    WE_VALIDATE_RENDER(m_Initialized, "DepthTarget::Resize", "DepthTarget not initialized.");
    if (width == m_Width && height == m_Height) {
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }

    Shutdown();
    m_Width = width;
    m_Height = height;
    CreateResources();
    m_Initialized = true;
}

void DepthTarget::CreateResources() {
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

#endif // WE_HAS_VULKAN
