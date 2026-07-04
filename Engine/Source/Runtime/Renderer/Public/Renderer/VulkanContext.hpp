#pragma once

#if WE_HAS_VULKAN
#include "Renderer/Export.hpp"
#include <volk.h>
#endif
#if WE_HAS_SDL3
#include <SDL3/SDL.h>
#endif
#include <string>
#include <vector>
#include <memory>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

class VulkanContext {
public:
    RENDERER_API VulkanContext(SDL_Window* window);
    RENDERER_API ~VulkanContext();

    // Prevent copying/assignment
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    VkInstance GetInstance() const { return m_Instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkDevice GetDevice() const { return m_Device; }
    VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    uint32_t GetGraphicsQueueFamily() const { return m_GraphicsFamily; }
    uint32_t GetPresentQueueFamily() const { return m_PresentFamily; }
    VkSurfaceKHR GetSurface() const { return m_Surface; }
    VkCommandPool GetCommandPool() const { return m_CommandPool; }
    VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

    const std::string& GetGPUName() const { return m_GPUName; }
    const std::string& GetRendererName() const { return m_RendererName; }

    // Memory and resource creation helpers
    RENDERER_API uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    RENDERER_API void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    RENDERER_API void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    RENDERER_API VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    
    // Command buffer recording helpers
    RENDERER_API VkCommandBuffer BeginSingleTimeCommands() const;
    RENDERER_API void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    RENDERER_API void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;

private:
    void CreateInstance();
    void CreateSurface(SDL_Window* window);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateCommandPool();
    void CreateDescriptorPool();

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    uint32_t m_GraphicsFamily = 0xFFFFFFFF;
    uint32_t m_PresentFamily = 0xFFFFFFFF;

    std::string m_GPUName;
    std::string m_RendererName;
};

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
