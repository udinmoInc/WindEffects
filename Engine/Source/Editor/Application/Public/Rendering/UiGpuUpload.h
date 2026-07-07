#pragma once

#include <volk.h>
#include <functional>

namespace we::runtime::renderer {
class DeviceContext;
class ResourceManager;
}

namespace we::UI {

// One-shot command buffer uploads shared by UI texture resources.
class UiGpuUpload {
public:
    void Init(we::runtime::renderer::DeviceContext* device, we::runtime::renderer::ResourceManager* resources);
    void Shutdown();

    void SubmitOneTime(const std::function<void(VkCommandBuffer)>& record);

    we::runtime::renderer::DeviceContext* GetDeviceContext() const { return m_Device; }
    we::runtime::renderer::ResourceManager* GetResourceManager() const { return m_Resources; }
    VkDevice GetDevice() const;

private:
    we::runtime::renderer::DeviceContext* m_Device = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkFence m_Fence = VK_NULL_HANDLE;
};

} // namespace we::UI
