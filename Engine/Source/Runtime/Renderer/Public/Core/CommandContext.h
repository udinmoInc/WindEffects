#pragma once

#include <volk.h>
#include <vector>

namespace we::runtime::renderer {

class DeviceContext;

struct CommandContextConfig {
    DeviceContext* deviceContext = nullptr;
    uint32_t maxFramesInFlight = 2;
};

class CommandContext {
public:
    CommandContext() = default;
    ~CommandContext();

    CommandContext(const CommandContext&) = delete;
    CommandContext& operator=(const CommandContext&) = delete;

    void Init(const CommandContextConfig& config);
    void Shutdown();

    VkCommandBuffer BeginFrame(uint32_t frameIndex);
    void EndFrame(uint32_t frameIndex);

    VkCommandBuffer GetCurrentCommandBuffer(uint32_t frameIndex) const;

private:
    DeviceContext* m_DeviceContext = nullptr;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
