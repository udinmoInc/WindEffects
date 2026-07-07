#pragma once

#include "Renderer/Export.h"
#include <volk.h>
#include <memory>

namespace we::runtime::renderer {

class DeviceContext;

struct ResourceManagerConfig {
    DeviceContext* deviceContext = nullptr;
};

class RENDERER_API ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager();

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    void Init(const ResourceManagerConfig& config);
    void Shutdown();

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

private:
    DeviceContext* m_DeviceContext = nullptr;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
