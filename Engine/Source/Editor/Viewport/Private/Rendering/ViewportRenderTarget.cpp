#include "Rendering/ViewportRenderTarget.h"

#if WE_HAS_VULKAN

#include "Core/DeviceContext.h"
#include "Resource/DepthTarget.h"
#include "Resource/ResourceManager.h"

namespace we::editor::viewport {

ViewportRenderTarget::~ViewportRenderTarget() {
    Shutdown();
}

void ViewportRenderTarget::Init(we::runtime::renderer::DeviceContext* deviceContext,
                                we::runtime::renderer::ResourceManager* resourceManager,
                                VkFormat colorFormat) {
    m_DeviceContext = deviceContext;
    m_ResourceManager = resourceManager;
    m_Device = deviceContext ? deviceContext->GetDevice() : VK_NULL_HANDLE;
    m_ColorFormat = colorFormat;
    m_DepthTarget = std::make_unique<we::runtime::renderer::DepthTarget>();
}

void ViewportRenderTarget::Shutdown() {
    DestroyColorResources();
    if (m_DepthTarget) {
        m_DepthTarget->Shutdown();
    }
    m_DepthTarget.reset();
    m_DeviceContext = nullptr;
    m_ResourceManager = nullptr;
    m_Device = VK_NULL_HANDLE;
    m_ColorFormat = VK_FORMAT_UNDEFINED;
}

void ViewportRenderTarget::Resize(uint32_t width, uint32_t height) {
    if (!m_ResourceManager || m_Device == VK_NULL_HANDLE) {
        return;
    }

    if (width == 0 || height == 0) {
        DestroyColorResources();
        if (m_DepthTarget) {
            m_DepthTarget->Shutdown();
        }
        m_Width = 0;
        m_Height = 0;
        return;
    }

    if (m_Width == width && m_Height == height && m_ColorImageView != VK_NULL_HANDLE) {
        return;
    }

    DestroyColorResources();

    m_Width = width;
    m_Height = height;
    CreateColorResources();

    if (m_DepthTarget) {
        we::runtime::renderer::DepthTargetConfig depthConfig{};
        depthConfig.deviceContext = m_DeviceContext;
        depthConfig.resourceManager = m_ResourceManager;
        depthConfig.width = width;
        depthConfig.height = height;

        if (m_DepthTarget->GetImage() == VK_NULL_HANDLE) {
            m_DepthTarget->Init(depthConfig);
        } else {
            m_DepthTarget->Resize(width, height);
        }
    }
}

void ViewportRenderTarget::DestroyColorResources() {
    if (!m_ResourceManager) {
        return;
    }

    m_ResourceManager->DestroyImageView(m_ColorImageView);
    m_ResourceManager->DestroyImage(m_ColorImage, m_ColorMemory);
}

void ViewportRenderTarget::CreateColorResources() {
    if (!m_ResourceManager || m_Device == VK_NULL_HANDLE) {
        return;
    }

    m_ResourceManager->CreateImage(
        m_Width,
        m_Height,
        m_ColorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_ColorImage,
        m_ColorMemory);

    m_ColorImageView = m_ResourceManager->CreateImageView(m_ColorImage, m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

} // namespace we::editor::viewport

#endif // WE_HAS_VULKAN

