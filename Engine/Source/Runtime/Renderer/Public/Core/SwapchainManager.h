#pragma once

#include <volk.h>
#include <vector>

#include "Platform/NativeHandle.h"
#include "Platform/Types.h"

namespace we::runtime::renderer {

class DeviceContext;
class FrameSync;

struct SwapchainManagerConfig {
    DeviceContext* deviceContext = nullptr;
    we::platform::WindowId window = we::platform::WindowId::Invalid;
    we::platform::NativeWindowHandle nativeWindow{};
    bool vsync = true;
};

class SwapchainManager {
public:
    SwapchainManager() = default;
    ~SwapchainManager();

    SwapchainManager(const SwapchainManager&) = delete;
    SwapchainManager& operator=(const SwapchainManager&) = delete;

    void Init(const SwapchainManagerConfig& config);
    void Shutdown();
    void Recreate();

    void SetFrameSync(FrameSync* frameSync) { m_FrameSync = frameSync; }

    VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
    VkFormat GetImageFormat() const { return m_ImageFormat; }
    VkExtent2D GetExtent() const { return m_Extent; }
    const std::vector<VkImage>& GetImages() const { return m_Images; }
    const std::vector<VkImageView>& GetImageViews() const { return m_ImageViews; }

    bool AcquireNextImage(uint32_t frameIndex, uint32_t& outImageIndex);
    void Present(uint32_t frameIndex, uint32_t imageIndex);
    void SubmitFrame(uint32_t frameIndex, uint32_t imageIndex, VkCommandBuffer commandBuffer);

    VkQueue GetPresentQueue() const { return m_PresentQueue; }
    uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }

private:
    void CreateSurface();
    void CreateSwapchain();
    void CreateImageViews();
    void CleanupSwapchain();

    SwapchainManagerConfig m_Config;
    FrameSync* m_FrameSync = nullptr;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_Extent = {};

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;

    uint32_t m_PresentQueueFamily = ~0u;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
