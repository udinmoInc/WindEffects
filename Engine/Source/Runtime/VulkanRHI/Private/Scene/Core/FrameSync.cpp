#include "Core/FrameSync.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"

namespace we::runtime::renderer {

FrameSync::~FrameSync() {
    Shutdown();
}

void FrameSync::Init(const FrameSyncConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "FrameSync", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "FrameSync", "DeviceContext is null.");
    WE_VALIDATE_INIT(config.maxFramesInFlight > 0, "FrameSync", "maxFramesInFlight must be > 0.");

    m_DeviceContext = config.deviceContext;

    VkDevice device = m_DeviceContext->GetDevice();

    m_ImageAvailableSemaphores.resize(config.maxFramesInFlight);
    m_RenderFinishedSemaphores.resize(config.maxFramesInFlight);
    m_InFlightFences.resize(config.maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < config.maxFramesInFlight; ++i) {
        VkResult result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]);
        WE_VALIDATE_INIT(result == VK_SUCCESS, "FrameSync", "Failed to create image-available semaphore.");

        result = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]);
        WE_VALIDATE_INIT(result == VK_SUCCESS, "FrameSync", "Failed to create render-finished semaphore.");

        result = vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]);
        WE_VALIDATE_INIT(result == VK_SUCCESS, "FrameSync", "Failed to create in-flight fence.");
    }

    m_Initialized = true;
}

void FrameSync::Shutdown() {
    if (!m_Initialized) return;

    VkDevice device = m_DeviceContext->GetDevice();
    vkDeviceWaitIdle(device);

    for (auto semaphore : m_ImageAvailableSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (auto semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    for (auto fence : m_InFlightFences) {
        vkDestroyFence(device, fence, nullptr);
    }

    m_ImageAvailableSemaphores.clear();
    m_RenderFinishedSemaphores.clear();
    m_InFlightFences.clear();
    m_Initialized = false;
}

VkFence FrameSync::GetInFlightFence(uint32_t frameIndex) const {
    return m_InFlightFences[frameIndex];
}

VkSemaphore FrameSync::GetImageAvailableSemaphore(uint32_t frameIndex) const {
    return m_ImageAvailableSemaphores[frameIndex];
}

VkSemaphore FrameSync::GetRenderFinishedSemaphore(uint32_t frameIndex) const {
    return m_RenderFinishedSemaphores[frameIndex];
}

void FrameSync::WaitForFrame(uint32_t frameIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "FrameSync::WaitForFrame", "FrameSync not initialized.");
    WE_VALIDATE_RENDER(frameIndex < m_InFlightFences.size(), "FrameSync::WaitForFrame", "Invalid frame index.");

    VkDevice device = m_DeviceContext->GetDevice();
    VkResult result = vkWaitForFences(device, 1, &m_InFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
    WE_VALIDATE_RENDER(result == VK_SUCCESS, "FrameSync::WaitForFrame", "Failed to wait for fence.");
}

void FrameSync::ResetFence(uint32_t frameIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "FrameSync::ResetFence", "FrameSync not initialized.");
    WE_VALIDATE_RENDER(frameIndex < m_InFlightFences.size(), "FrameSync::ResetFence", "Invalid frame index.");

    VkDevice device = m_DeviceContext->GetDevice();
    VkResult result = vkResetFences(device, 1, &m_InFlightFences[frameIndex]);
    WE_VALIDATE_RENDER(result == VK_SUCCESS, "FrameSync::ResetFence", "Failed to reset fence.");
}

} // namespace we::runtime::renderer
