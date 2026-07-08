#pragma once

#include <volk.h>
#include <cstdint>
#include <memory>
#include "Resource/DepthTarget.h"

namespace we::runtime::renderer {
class ResourceManager;
class DeviceContext;
}

namespace we::editor::viewport {

class ViewportRenderTarget {
public:
    ViewportRenderTarget() = default;
    ~ViewportRenderTarget();

    ViewportRenderTarget(const ViewportRenderTarget&) = delete;
    ViewportRenderTarget& operator=(const ViewportRenderTarget&) = delete;

    void Init(we::runtime::renderer::DeviceContext* deviceContext,
              we::runtime::renderer::ResourceManager* resourceManager,
              VkFormat colorFormat);
    void Shutdown();

    void Resize(uint32_t width, uint32_t height);

    VkImage GetColorImage() const { return m_ColorImage; }
    VkImageView GetColorImageView() const { return m_ColorImageView; }
    VkFormat GetColorFormat() const { return m_ColorFormat; }
    we::runtime::renderer::DepthTarget* GetDepthTarget() const { return m_DepthTarget.get(); }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void DestroyColorResources();
    void CreateColorResources();

    we::runtime::renderer::DeviceContext* m_DeviceContext = nullptr;
    we::runtime::renderer::ResourceManager* m_ResourceManager = nullptr;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;
    std::unique_ptr<we::runtime::renderer::DepthTarget> m_DepthTarget;

    VkImage m_ColorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_ColorMemory = VK_NULL_HANDLE;
    VkImageView m_ColorImageView = VK_NULL_HANDLE;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

} // namespace we::editor::viewport

