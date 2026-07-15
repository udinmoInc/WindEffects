#include "Text/Rendering/TextGpuBackend.h"

#include <memory>

namespace we::runtime::text::rendering {
namespace {

class NullTextGpuBackend final : public ITextGpuBackend {
public:
    bool Initialize(const GpuBackendConfig&) override { return true; }
    void Shutdown() override {}
    AtlasGpuHandle UploadAtlas(const AtlasPage& page) override {
        m_Stats.textureMemoryBytes += page.rgba.size();
        return ++m_NextHandle;
    }
    void DrawBatches(
        CommandBufferHandle,
        const std::span<const TextDrawBatch> batches,
        float,
        float) override
    {
        for (const TextDrawBatch& batch : batches) {
            ++m_Stats.drawCalls;
            m_Stats.verticesRendered += batch.vertices.size();
        }
    }
    TextGpuStats GetStats() const override { return m_Stats; }
    GraphicsApi GetApi() const override { return GraphicsApi::Unknown; }

private:
    AtlasGpuHandle m_NextHandle = 1;
    TextGpuStats m_Stats;
};

class ApiTaggedBackend final : public ITextGpuBackend {
public:
    ApiTaggedBackend(GraphicsApi api, std::unique_ptr<ITextGpuBackend> inner)
        : m_Api(api)
        , m_Inner(std::move(inner))
    {
    }

    bool Initialize(const GpuBackendConfig& config) override { return m_Inner->Initialize(config); }
    void Shutdown() override { m_Inner->Shutdown(); }
    AtlasGpuHandle UploadAtlas(const AtlasPage& page) override { return m_Inner->UploadAtlas(page); }
    void DrawBatches(
        CommandBufferHandle commandBuffer,
        const std::span<const TextDrawBatch> batches,
        float viewportWidth,
        float viewportHeight) override
    {
        m_Inner->DrawBatches(commandBuffer, batches, viewportWidth, viewportHeight);
    }
    TextGpuStats GetStats() const override { return m_Inner->GetStats(); }
    GraphicsApi GetApi() const override { return m_Api; }

private:
    GraphicsApi m_Api;
    std::unique_ptr<ITextGpuBackend> m_Inner;
};

} // namespace

std::unique_ptr<ITextGpuBackend> CreateTextGpuBackend(const GraphicsApi api) {
    switch (api) {
    case GraphicsApi::Vulkan:
        return std::make_unique<ApiTaggedBackend>(GraphicsApi::Vulkan, std::make_unique<NullTextGpuBackend>());
    case GraphicsApi::DirectX12:
        return std::make_unique<ApiTaggedBackend>(GraphicsApi::DirectX12, std::make_unique<NullTextGpuBackend>());
    case GraphicsApi::Metal:
        return std::make_unique<ApiTaggedBackend>(GraphicsApi::Metal, std::make_unique<NullTextGpuBackend>());
    case GraphicsApi::OpenGL:
        return std::make_unique<ApiTaggedBackend>(GraphicsApi::OpenGL, std::make_unique<NullTextGpuBackend>());
    default:
        return std::make_unique<NullTextGpuBackend>();
    }
}

} // namespace we::runtime::text::rendering
