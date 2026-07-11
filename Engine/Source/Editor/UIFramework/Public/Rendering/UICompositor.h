#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <volk.h>
#include <memory>

namespace WindEffects::Editor::UI {

// UI compositor for final overlay pass (UE5 Slate-inspired)
class UIFRAMEWORK_API UICompositor {
public:
    UICompositor();
    ~UICompositor();

    bool Initialize(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   VkFormat swapchainFormat);
    void Shutdown();

    // Compositing
    void BeginComposite(VkCommandBuffer cmd, VkImageView swapchainView, 
                       uint32_t width, uint32_t height,
                       VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD);
    void EndComposite(VkCommandBuffer cmd);

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

} // namespace WindEffects::Editor::UI
