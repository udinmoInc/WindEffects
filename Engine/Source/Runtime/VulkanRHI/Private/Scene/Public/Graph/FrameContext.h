#pragma once

#include "Resource/VulkanDepthTarget.h"
#include <volk.h>
#include <cstdint>

namespace we::runtime::renderer {

class CameraSystem;
class DirectionalLight;
class EnvironmentUniform;
class SceneRenderer;

struct FrameContext {
    uint32_t frameIndex = 0;
    uint32_t imageIndex = 0;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkExtent2D extent{};
    VkRect2D sceneRenderArea{};
    VkImage colorImage = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    CameraSystem* camera = nullptr;
    DirectionalLight* directionalLight = nullptr;
    EnvironmentUniform* environmentUniform = nullptr;
    VulkanDepthTarget* depthTarget = nullptr;
    SceneRenderer* sceneRenderer = nullptr;
};

} // namespace we::runtime::renderer
