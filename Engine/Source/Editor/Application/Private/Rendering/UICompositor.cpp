#include "Rendering/UICompositor.h"
#include "Core/Logger.h"

namespace we::UI {

UICompositor::UICompositor()
    : m_Device(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_SwapchainFormat(VK_FORMAT_UNDEFINED)
    , m_Width(0)
    , m_Height(0)
{
}

UICompositor::~UICompositor() {
    Shutdown();
}

bool UICompositor::Initialize(VkDevice device,
                               VkPhysicalDevice physicalDevice,
                               VkFormat swapchainFormat) {
    m_Device = device;
    m_PhysicalDevice = physicalDevice;
    m_SwapchainFormat = swapchainFormat;
    
    CreateRenderPass();
    
    return true;
}

void UICompositor::Shutdown() {
    if (m_Framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(m_Device, m_Framebuffer, nullptr);
        m_Framebuffer = VK_NULL_HANDLE;
    }
    
    if (m_RenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
    }
}

void UICompositor::BeginComposite(VkCommandBuffer cmd, VkImageView swapchainView,
                                   uint32_t width, uint32_t height,
                                   VkAttachmentLoadOp loadOp) {
    m_Width = width;
    m_Height = height;
    
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchainView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = loadOp;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = {{0.09f, 0.09f, 0.09f, 1.0f}};
    
    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = {width, height};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    
    vkCmdBeginRendering(cmd, &renderingInfo);
}

void UICompositor::EndComposite(VkCommandBuffer cmd) {
    vkCmdEndRendering(cmd);
}

void UICompositor::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void UICompositor::SetViewport(float x, float y, float width, float height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void UICompositor::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_SwapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
        HE_ERROR("UICompositor: Failed to create render pass");
    }
}

void UICompositor::CreateFramebuffer() {
}

} // namespace we::UI
