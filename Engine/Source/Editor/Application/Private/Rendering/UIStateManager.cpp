#include "Rendering/UIStateManager.hpp"
#include "Core/Logger.hpp"

namespace we::UI {

UIStateManager::UIStateManager()
    : m_Device(VK_NULL_HANDLE)
{
}

UIStateManager::~UIStateManager() {
    Shutdown();
}

void UIStateManager::Initialize(VkDevice device) {
    m_Device = device;
}

void UIStateManager::Shutdown() {
    m_Device = VK_NULL_HANDLE;
}

void UIStateManager::SaveState(VkCommandBuffer cmd, SavedVulkanState& outState) {
    // Note: Vulkan doesn't provide direct state query commands
    // This is a simplified implementation that assumes we track state
    // In a real implementation, you would track state changes or use
    // secondary command buffers for isolation
    
    outState.boundPipeline = VK_NULL_HANDLE; // Would need to track this
    outState.pipelineLayout = VK_NULL_HANDLE;
    outState.descriptorSetCount = 0;
    outState.viewportValid = false;
    outState.scissorValid = false;
    
    // Reset dynamic state tracking
    for (int i = 0; i < 4; ++i) {
        outState.blendConstants[i] = 0.0f;
    }
    outState.depthBias[0] = 0.0f;
    outState.depthBias[1] = 0.0f;
    outState.depthBounds[0] = 0.0f;
    outState.depthBounds[1] = 1.0f;
    outState.stencilCompareMask = 0xFFFFFFFF;
    outState.stencilWriteMask = 0xFFFFFFFF;
    outState.stencilReference = 0;
}

void UIStateManager::RestoreState(VkCommandBuffer cmd, const SavedVulkanState& state) {
    // Restore pipeline if it was bound
    if (state.boundPipeline != VK_NULL_HANDLE && state.pipelineLayout != VK_NULL_HANDLE) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state.boundPipeline);
    }
    
    // Restore descriptor sets
    if (state.descriptorSetCount > 0) {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                               state.pipelineLayout, 0, state.descriptorSetCount,
                               state.descriptorSets, 0, nullptr);
    }
    
    // Restore viewport
    if (state.viewportValid) {
        vkCmdSetViewport(cmd, 0, 1, &state.viewport);
    }
    
    // Restore scissor
    if (state.scissorValid) {
        vkCmdSetScissor(cmd, 0, 1, &state.scissor);
    }
    
    // Restore blend constants
    vkCmdSetBlendConstants(cmd, state.blendConstants);
    
    // Restore depth bias
    vkCmdSetDepthBias(cmd, state.depthBias[0], state.depthBias[1], 0.0f);
    
    // Restore depth bounds
    vkCmdSetDepthBounds(cmd, state.depthBounds[0], state.depthBounds[1]);
    
    // Restore stencil state
    vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, state.stencilCompareMask);
    vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, state.stencilWriteMask);
    vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_AND_BACK, state.stencilReference);
}

bool UIStateManager::ValidateState(const SavedVulkanState& state) const {
    // Basic validation
    if (state.descriptorSetCount > 8) {
        return false;
    }
    
    return true;
}

} // namespace we::UI
