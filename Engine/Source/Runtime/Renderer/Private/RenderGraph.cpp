#include "Renderer/RenderGraph.hpp"
#include <array>

namespace we::runtime::renderer {

#if WE_HAS_VULKAN

RenderGraph::RenderGraph(const std::shared_ptr<Renderer>& renderer)
    : m_Renderer(renderer) {}

void RenderGraph::BeginOffscreenPass(VkCommandBuffer cmd) const {
    Framebuffer& offscreenFB = m_Renderer->GetOffscreenFramebuffer();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_Renderer->GetOffscreenRenderPass();
    renderPassInfo.framebuffer = offscreenFB.GetFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { offscreenFB.GetWidth(), offscreenFB.GetHeight() };

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    // Reverse-Z: clear to far depth (0.0), geometry writes toward 1.0 near camera.
    clearValues[1].depthStencil = { 0.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport dynamically
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(offscreenFB.GetWidth());
    viewport.height = static_cast<float>(offscreenFB.GetHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    // Set scissor dynamically
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { offscreenFB.GetWidth(), offscreenFB.GetHeight() };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void RenderGraph::EndOffscreenPass(VkCommandBuffer cmd) const {
    vkCmdEndRenderPass(cmd);
}

void RenderGraph::BeginSwapchainPass(VkCommandBuffer cmd) const {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_Renderer->GetSwapchainRenderPass();
    renderPassInfo.framebuffer = m_Renderer->GetSwapchainFramebuffer(m_Renderer->GetCurrentImageIndex());
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = { m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight() };

    VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport dynamically
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_Renderer->GetSwapchainWidth());
    viewport.height = static_cast<float>(m_Renderer->GetSwapchainHeight());
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    // Set scissor dynamically
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight() };
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void RenderGraph::EndSwapchainPass(VkCommandBuffer cmd) const {
    vkCmdEndRenderPass(cmd);
}

#endif // WE_HAS_VULKAN

} // namespace we::runtime::renderer
