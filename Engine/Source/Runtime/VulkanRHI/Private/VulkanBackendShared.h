#pragma once

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
class SwapchainManager;
class CommandContext;
} // namespace we::runtime::renderer

namespace we::rhi::vulkan {

// Scene + UI Vulkan backends share one legacy device context created by the scene backend.
struct VulkanBackendShared {
    static VulkanBackendShared& Get();

    we::runtime::renderer::DeviceContext* deviceContext = nullptr;
    we::runtime::renderer::ResourceManager* resourceManager = nullptr;
    we::runtime::renderer::SwapchainManager* swapchain = nullptr;
    we::runtime::renderer::CommandContext* commands = nullptr;
};

} // namespace we::rhi::vulkan
