#pragma once

#if WE_HAS_VULKAN

#include <volk.h>
#include <cstdint>
#include "Renderer/Export.h"

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

struct DepthTargetConfig {
    DeviceContext* deviceContext = nullptr;
    ResourceManager* resourceManager = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

class RENDERER_API DepthTarget {
public:
    DepthTarget() = default;
    ~DepthTarget();

    DepthTarget(const DepthTarget&) = delete;
    DepthTarget& operator=(const DepthTarget&) = delete;

    void Init(const DepthTargetConfig& config);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    VkImage GetImage() const { return m_Image; }
    VkImageView GetImageView() const { return m_ImageView; }
    VkFormat GetFormat() const { return m_Format; }
    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

private:
    void CreateResources();

    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;

    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkFormat m_Format = VK_FORMAT_D32_SFLOAT;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer

#endif // WE_HAS_VULKAN
