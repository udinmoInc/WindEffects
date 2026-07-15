#include "Core/CommandContext.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include <stdexcept>

namespace we::runtime::renderer {

CommandContext::~CommandContext() {
    Shutdown();
}

void CommandContext::Init(const CommandContextConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "CommandContext", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "CommandContext", "DeviceContext is null.");

    m_DeviceContext = config.deviceContext;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_DeviceContext->GetGraphicsQueueFamily();

    VkResult result = vkCreateCommandPool(m_DeviceContext->GetDevice(), &poolInfo, nullptr, &m_CommandPool);
    WE_VALIDATE_INIT(result == VK_SUCCESS, "CommandContext", "Failed to create command pool.");

    m_CommandBuffers.resize(config.maxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

    result = vkAllocateCommandBuffers(m_DeviceContext->GetDevice(), &allocInfo, m_CommandBuffers.data());
    WE_VALIDATE_INIT(result == VK_SUCCESS, "CommandContext", "Failed to allocate command buffers.");

    m_Initialized = true;
}

void CommandContext::Shutdown() {
    if (!m_Initialized) return;

    if (m_CommandPool && m_DeviceContext && m_DeviceContext->GetDevice()) {
        vkDestroyCommandPool(m_DeviceContext->GetDevice(), m_CommandPool, nullptr);
        m_CommandPool = VK_NULL_HANDLE;
    }

    m_Initialized = false;
}

VkCommandBuffer CommandContext::BeginFrame(uint32_t frameIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "BeginFrame", "CommandContext not initialized.");
    WE_VALIDATE_RENDER(frameIndex < m_CommandBuffers.size(), "BeginFrame", "Invalid frame index.");

    VkCommandBuffer cmd = m_CommandBuffers[frameIndex];

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult result = vkBeginCommandBuffer(cmd, &beginInfo);
    WE_VALIDATE_RENDER(result == VK_SUCCESS, "BeginFrame", "Failed to begin recording command buffer.");

    return cmd;
}

void CommandContext::EndFrame(uint32_t frameIndex) {
    WE_VALIDATE_RENDER(m_Initialized, "EndFrame", "CommandContext not initialized.");
    WE_VALIDATE_RENDER(frameIndex < m_CommandBuffers.size(), "EndFrame", "Invalid frame index.");

    VkCommandBuffer cmd = m_CommandBuffers[frameIndex];

    VkResult result = vkEndCommandBuffer(cmd);
    WE_VALIDATE_RENDER(result == VK_SUCCESS, "EndFrame", "Failed to end command buffer.");
}

VkCommandBuffer CommandContext::GetCurrentCommandBuffer(uint32_t frameIndex) const {
    return m_CommandBuffers[frameIndex];
}

} // namespace we::runtime::renderer
