#pragma once

#include "Application/Export.hpp"

#include <volk.h>
#include <array>

namespace we::UI {

// Saved Vulkan state for restoration
struct APPLICATION_API SavedVulkanState {
    VkPipeline boundPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSets[8] = {VK_NULL_HANDLE};
    uint32_t descriptorSetCount = 0;
    VkViewport viewport;
    VkRect2D scissor;
    uint32_t dynamicStateCount = 0;
    
    // Dynamic state
    float blendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float depthBias[2] = {0.0f, 0.0f};
    float depthBounds[2] = {0.0f, 1.0f};
    uint32_t stencilCompareMask = 0xFFFFFFFF;
    uint32_t stencilWriteMask = 0xFFFFFFFF;
    uint32_t stencilReference = 0;
    
    bool viewportValid = false;
    bool scissorValid = false;
};

// Vulkan state manager for save/restore (UE5 Slate-inspired)
class APPLICATION_API UIStateManager {
public:
    UIStateManager();
    ~UIStateManager();

    void Initialize(VkDevice device);
    void Shutdown();

    // State save/restore
    void SaveState(VkCommandBuffer cmd, SavedVulkanState& outState);
    void RestoreState(VkCommandBuffer cmd, const SavedVulkanState& state);

    // State validation
    bool ValidateState(const SavedVulkanState& state) const;

private:
    VkDevice m_Device;
};

} // namespace we::UI
