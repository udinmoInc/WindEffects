#pragma once

#include "Application/Export.h"

#include <volk.h>
#include <memory>

namespace we::UI {

// UI compositor for final overlay pass (UE5 Slate-inspired)
class APPLICATION_API UICompositor {
public:
    UICompositor();
    ~UICompositor();

    bool Initialize(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   VkFormat swapchainFormat);
    void Shutdown();

    // Compositing
    void BeginComposite(VkCommandBuffer cmd, VkImageView swapchainView, 
                       uint32_t width, uint32_t height);
    void EndComposite(VkCommandBuffer cmd);

    // State management
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height);
    void SetViewport(float x, float y, float width, float height);

private:
    void CreateRenderPass();
    void CreateFramebuffer();

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkFormat m_SwapchainFormat;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

    uint32_t m_Width;
    uint32_t m_Height;
};

} // namespace we::UI
