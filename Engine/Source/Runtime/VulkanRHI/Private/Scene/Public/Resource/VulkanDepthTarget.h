#pragma once

#include <volk.h>
#include <cstdint>

namespace we::runtime::renderer {

class DeviceContext;
class ResourceManager;

class VulkanDepthTarget {
public:
    VulkanDepthTarget() = default;
    ~VulkanDepthTarget();

    VulkanDepthTarget(const VulkanDepthTarget&) = delete;
    VulkanDepthTarget& operator=(const VulkanDepthTarget&) = delete;

    void Init(DeviceContext* deviceContext, ResourceManager* resourceManager,
              uint32_t width, uint32_t height, VkFormat format);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    [[nodiscard]] VkImage GetImage() const { return m_Image; }
    [[nodiscard]] VkImageView GetImageView() const { return m_ImageView; }
    [[nodiscard]] VkFormat GetFormat() const { return m_Format; }
    [[nodiscard]] uint32_t GetWidth() const { return m_Width; }
    [[nodiscard]] uint32_t GetHeight() const { return m_Height; }
    [[nodiscard]] bool IsValid() const { return m_Image != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE; }

private:
    void CreateResources();

    DeviceContext* m_DeviceContext = nullptr;
    ResourceManager* m_ResourceManager = nullptr;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
};

} // namespace we::runtime::renderer
