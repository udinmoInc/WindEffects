#include "Rendering/UiGpuUpload.h"
#include "Core/DeviceContext.h"
#include "Core/Logger.h"
#include "Resource/ResourceManager.h"

namespace WindEffects::Editor::UI {

void UiGpuUpload::Init(we::runtime::renderer::DeviceContext* device, we::runtime::renderer::ResourceManager* resources) {
    m_Device = device;
    m_Resources = resources;
    if (!m_Device || !m_Resources) {
        return;
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = m_Device->GetGraphicsQueueFamily();
    vkCreateCommandPool(m_Device->GetDevice(), &poolInfo, nullptr, &m_CommandPool);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_Device->GetDevice(), &fenceInfo, nullptr, &m_Fence);
}

void UiGpuUpload::Shutdown() {
    if (!m_Device) {
        return;
    }

    VkDevice device = m_Device->GetDevice();
    if (m_Fence) {
        vkDestroyFence(device, m_Fence, nullptr);
        m_Fence = VK_NULL_HANDLE;
    }
    if (m_CommandPool) {
        vkDestroyCommandPool(device, m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }
    m_Device = nullptr;
    m_Resources = nullptr;
}

VkDevice UiGpuUpload::GetDevice() const {
    return m_Device ? m_Device->GetDevice() : VK_NULL_HANDLE;
}

void UiGpuUpload::SubmitOneTime(const std::function<void(VkCommandBuffer)>& record) {
    if (!m_Device || !m_Resources || !record) {
        return;
    }

    VkDevice device = m_Device->GetDevice();

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    record(cmd);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    const VkResult submitResult = vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_Fence);
    if (submitResult != VK_SUCCESS) {
        HE_ERROR("[UiGpuUpload] vkQueueSubmit failed: " + std::to_string(static_cast<int>(submitResult)));
        vkFreeCommandBuffers(device, m_CommandPool, 1, &cmd);
        return;
    }

    const VkResult waitResult = vkWaitForFences(device, 1, &m_Fence, VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS) {
        HE_ERROR("[UiGpuUpload] vkWaitForFences failed: " + std::to_string(static_cast<int>(waitResult)));
        vkFreeCommandBuffers(device, m_CommandPool, 1, &cmd);
        return;
    }

    vkResetFences(device, 1, &m_Fence);
    vkFreeCommandBuffers(device, m_CommandPool, 1, &cmd);
}

} // namespace WindEffects::Editor::UI
