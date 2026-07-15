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
    bool independentBlend = true;
    bool depthClamp = false;
    bool fillModeNonSolid = true;
    bool samplerAnisotropy = true;
    bool textureCompressionBC = false;
    bool shaderFloat64 = false;
    bool shaderInt64 = false;
    bool drawIndirectFirstInstance = false;
    bool hostVisibleDeviceLocal = false;
    uint32_t maxFramesInFlight = 2;
    uint32_t maxColorAttachments = 8;
    uint32_t maxPushConstantBytes = 128;
    uint32_t maxBoundDescriptorSets = 4;
    uint32_t maxDescriptorSetSamplers = 16;
    uint32_t maxDescriptorSetUniformBuffers = 16;
    uint32_t maxDescriptorSetStorageBuffers = 16;
    uint32_t maxDescriptorSetSampledImages = 16;
    uint32_t maxDescriptorSetStorageImages = 8;
    uint32_t maxComputeWorkGroupInvocations = 1024;
    uint32_t maxComputeWorkGroupSizeX = 1024;
    uint32_t maxComputeWorkGroupSizeY = 1024;
    uint32_t maxComputeWorkGroupSizeZ = 64;
    uint32_t maxTextureDimension2D = 16384;
    uint64_t minUniformBufferOffsetAlignment = 256;
    uint64_t minStorageBufferOffsetAlignment = 16;
    float maxSamplerAnisotropy = 16.0f;

    [[nodiscard]] bool SupportsMeshShaders() const noexcept { return meshShaders; }
    [[nodiscard]] bool SupportsRayTracing() const noexcept { return rayTracing; }
    [[nodiscard]] bool SupportsVariableRateShading() const noexcept { return variableRateShading; }
    [[nodiscard]] bool SupportsBindlessResources() const noexcept { return bindlessResources; }
    [[nodiscard]] bool SupportsTimelineSemaphores() const noexcept { return timelineSemaphores; }
    [[nodiscard]] bool SupportsAsyncCompute() const noexcept { return asyncCompute; }
    [[nodiscard]] bool SupportsSparseResources() const noexcept { return sparseResources; }
    [[nodiscard]] bool SupportsDynamicRendering() const noexcept { return dynamicRendering; }
    [[nodiscard]] bool SupportsMultiDrawIndirect() const noexcept { return multiDrawIndirect; }
    [[nodiscard]] bool SupportsAnisotropy() const noexcept { return samplerAnisotropy; }
};

} // namespace we::rhi
