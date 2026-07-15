#pragma once

#include "RHI/Export.h"

#include <cstdint>

namespace we::rhi {

struct RHI_API RHIMemoryStats {
    uint64_t dedicatedVideoMemoryBytes = 0;
    uint64_t allocatedBytes = 0;
    uint64_t usedBytes = 0;
    uint64_t bufferBytes = 0;
    uint64_t textureBytes = 0;
};

struct RHI_API RHIFrameStats {
    uint64_t frameIndex = 0;
    double cpuFrameMs = 0.0;
    double gpuFrameMs = 0.0;
    uint32_t drawCalls = 0;
    uint32_t dispatchCalls = 0;
    uint32_t triangles = 0;
    uint32_t pipelineBinds = 0;
    uint32_t barrierCount = 0;
};

struct RHI_API RHIDiagnostics {
    RHIMemoryStats memory{};
    RHIFrameStats lastFrame{};
    uint64_t resourcesCreated = 0;
    uint64_t resourcesDestroyed = 0;
    uint64_t deferredDestroys = 0;
    uint64_t validationErrors = 0;
    double deviceCreateMs = 0.0;
    double swapchainCreateMs = 0.0;

    void Reset() { *this = RHIDiagnostics{}; }
};

} // namespace we::rhi
