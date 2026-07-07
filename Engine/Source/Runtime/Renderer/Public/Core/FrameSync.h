#pragma once

#include <volk.h>
#include <cstdint>
#include <vector>

namespace we::runtime::renderer {

class DeviceContext;

struct FrameSyncConfig {
    DeviceContext* deviceContext = nullptr;
    uint32_t maxFramesInFlight = 2;
};

class FrameSync {
public:
    FrameSync() = default;
    ~FrameSync();

    FrameSync(const FrameSync&) = delete;
    FrameSync& operator=(const FrameSync&) = delete;

    void Init(const FrameSyncConfig& config);
    void Shutdown();

    uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
    VkFence GetInFlightFence(uint32_t frameIndex) const;
    VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
    VkSemaphore GetRenderFinishedSemaphore(uint32_t frameIndex) const;

    void WaitForFrame(uint32_t frameIndex);
    void ResetFence(uint32_t frameIndex);

private:
    DeviceContext* m_DeviceContext = nullptr;
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    uint32_t m_CurrentFrame = 0;
    bool m_Initialized = false;
};

} // namespace we::runtime::renderer
