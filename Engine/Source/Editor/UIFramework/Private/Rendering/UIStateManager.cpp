#include "Rendering/UIStateManager.h"
#include "Core/Logger.h"

namespace WindEffects::Editor::UI {

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
    (void)cmd;
    (void)state;
    // SaveState does not capture prior pipeline/descriptor/dynamic state.
    // Restoring guessed defaults was invoking optional vkCmd* entry points
    // that are not always loaded in the overlay module's volk table.
}

bool UIStateManager::ValidateState(const SavedVulkanState& state) const {
    // Basic validation
    if (state.descriptorSetCount > 8) {
        return false;
    }
    
    return true;
}

} // namespace WindEffects::Editor::UI
