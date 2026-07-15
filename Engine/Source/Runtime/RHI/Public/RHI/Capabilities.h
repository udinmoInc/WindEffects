#pragma once

#include "RHI/Export.h"

#include <cstdint>

namespace we::rhi {

struct RHI_API RHICapabilities {
    bool meshShaders = false;
    bool rayTracing = false;
    bool variableRateShading = false;
    bool bindlessResources = false;
    bool timelineSemaphores = false;
    bool asyncCompute = false;
    bool sparseResources = false;
    bool conservativeRasterization = false;
    bool samplerFeedback = false;
    bool dynamicRendering = true;
    bool multiview = false;
    bool multiDrawIndirect = false;
    bool geometryShaders = false;
    bool tessellation = false;
    bool timestamps = false;
    bool pipelineStatistics = false;
    bool debugMarkers = false;
    bool multipleWindows = true;
    uint32_t maxFramesInFlight = 2;
    uint32_t maxColorAttachments = 8;

    [[nodiscard]] bool SupportsMeshShaders() const noexcept { return meshShaders; }
    [[nodiscard]] bool SupportsRayTracing() const noexcept { return rayTracing; }
    [[nodiscard]] bool SupportsVariableRateShading() const noexcept { return variableRateShading; }
    [[nodiscard]] bool SupportsBindlessResources() const noexcept { return bindlessResources; }
    [[nodiscard]] bool SupportsTimelineSemaphores() const noexcept { return timelineSemaphores; }
    [[nodiscard]] bool SupportsAsyncCompute() const noexcept { return asyncCompute; }
    [[nodiscard]] bool SupportsSparseResources() const noexcept { return sparseResources; }
    [[nodiscard]] bool SupportsDynamicRendering() const noexcept { return dynamicRendering; }
    [[nodiscard]] bool SupportsMultiDrawIndirect() const noexcept { return multiDrawIndirect; }
};

} // namespace we::rhi
