#pragma once

#include "Text/Core/FontTypes.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"
#include "Text/Rendering/TextBatcher.h"

#include <cstdint>
#include <span>
#include <vector>

namespace we::runtime::text::rendering {

using AtlasGpuHandle = uint64_t;
using CommandBufferHandle = void*;

constexpr AtlasGpuHandle kInvalidAtlasGpuHandle = 0;

struct GpuBackendConfig {
    GraphicsApi api = GraphicsApi::Vulkan;
    void* device = nullptr;
    void* instance = nullptr;
    uint32_t backbufferWidth = 0;
    uint32_t backbufferHeight = 0;
};

struct TextGpuStats {
    size_t drawCalls = 0;
    size_t verticesRendered = 0;
    size_t textureMemoryBytes = 0;
};

class TEXT_API ITextGpuBackend {
public:
    virtual ~ITextGpuBackend() = default;
    [[nodiscard]] virtual bool Initialize(const GpuBackendConfig& config) = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual AtlasGpuHandle UploadAtlas(const AtlasPage& page) = 0;
    virtual void DrawBatches(
        CommandBufferHandle commandBuffer,
        std::span<const TextDrawBatch> batches,
        float viewportWidth,
        float viewportHeight) = 0;
    [[nodiscard]] virtual TextGpuStats GetStats() const = 0;
    [[nodiscard]] virtual GraphicsApi GetApi() const = 0;
};

[[nodiscard]] TEXT_API std::unique_ptr<ITextGpuBackend> CreateTextGpuBackend(GraphicsApi api);

} // namespace we::runtime::text::rendering
