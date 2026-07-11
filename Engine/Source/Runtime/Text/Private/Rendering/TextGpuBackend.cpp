#include "Text/Rendering/TextGpuBackend.h"

#include <unordered_map>

#include "Core/Logger.h"

#if defined(WE_HAS_VULKAN) && WE_HAS_VULKAN
#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"
#include "Shader/ShaderLibrary.h"
#include <volk.h>
#endif

namespace we::runtime::text::rendering {

namespace {

class NullTextGpuBackend final : public ITextGpuBackend {
public:
    bool Initialize(const GpuBackendConfig&) override { return true; }
    void Shutdown() override {}
    AtlasGpuHandle UploadAtlas(const AtlasPage& page) override
    {
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

#if defined(WE_HAS_VULKAN) && WE_HAS_VULKAN
class VulkanTextBackend final : public ITextGpuBackend {
public:
    bool Initialize(const GpuBackendConfig& config) override
    {
        m_DeviceContext = static_cast<we::runtime::renderer::DeviceContext*>(config.device);
        if (!m_DeviceContext) {
            WE_LOG_ERROR("Text", "VulkanTextBackend requires DeviceContext");
            return false;
        }
        m_ResourceManager = std::make_unique<we::runtime::renderer::ResourceManager>();
        we::runtime::renderer::ResourceManagerConfig resourceConfig;
        resourceConfig.deviceContext = m_DeviceContext;
        m_ResourceManager->Init(resourceConfig);
        m_Initialized = true;
        return true;
    }

    void Shutdown() override
    {
        if (m_ResourceManager) {
            m_ResourceManager->Shutdown();
            m_ResourceManager.reset();
        }
        m_UploadedAtlases.clear();
        m_Initialized = false;
    }

    AtlasGpuHandle UploadAtlas(const AtlasPage& page) override
    {
        if (!m_Initialized || !m_ResourceManager) {
            return kInvalidAtlasGpuHandle;
        }

        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        m_ResourceManager->CreateImage(
            page.width,
            page.height,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            image,
            memory);

        AtlasGpuHandle handle = ++m_NextHandle;
        m_UploadedAtlases.emplace(handle, UploadedAtlas{image, memory, page.width, page.height});
        m_Stats.textureMemoryBytes += page.rgba.size();
        return handle;
    }

    void DrawBatches(
        CommandBufferHandle commandBuffer,
        const std::span<const TextDrawBatch> batches,
        float viewportWidth,
        float viewportHeight) override
    {
        (void)commandBuffer;
        (void)viewportWidth;
        (void)viewportHeight;
        for (const TextDrawBatch& batch : batches) {
            if (batch.vertices.empty()) {
                continue;
            }
            ++m_Stats.drawCalls;
            m_Stats.verticesRendered += batch.vertices.size();
        }
    }

    TextGpuStats GetStats() const override { return m_Stats; }
    GraphicsApi GetApi() const override { return GraphicsApi::Vulkan; }

private:
    struct UploadedAtlas {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    we::runtime::renderer::DeviceContext* m_DeviceContext = nullptr;
    std::unique_ptr<we::runtime::renderer::ResourceManager> m_ResourceManager;
    std::unordered_map<AtlasGpuHandle, UploadedAtlas> m_UploadedAtlases;
    AtlasGpuHandle m_NextHandle = 1;
    TextGpuStats m_Stats;
    bool m_Initialized = false;
};
#endif

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

std::unique_ptr<ITextGpuBackend> CreateTextGpuBackend(const GraphicsApi api)
{
    switch (api) {
    case GraphicsApi::Vulkan:
#if defined(WE_HAS_VULKAN) && WE_HAS_VULKAN
        return std::make_unique<VulkanTextBackend>();
#else
        return std::make_unique<ApiTaggedBackend>(GraphicsApi::Vulkan, std::make_unique<NullTextGpuBackend>());
#endif
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
