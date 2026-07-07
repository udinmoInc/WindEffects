#pragma once

#include "Application/Export.hpp"

#include <volk.h>
#include <array>
#include <memory>
#include <vector>
#include "Core/Geometry.hpp"
#include "Core/PaintContext.hpp"
#include "Rendering/FontAtlas.hpp"
#include "Rendering/IconRenderer.hpp"
#include "Rendering/UiGpuUpload.h"

#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"
#include "Renderer/Renderer.h"

namespace we::UI {

class Widget;

struct APPLICATION_API UIVertex {
    float pos[2];
    float uv[2];
    float color[4];
    float sdfRect[4];
    float sdfParams[4];
};

class APPLICATION_API UIRenderer {
public:
    UIRenderer() = default;
    ~UIRenderer();

    bool Init(we::runtime::renderer::DeviceContext* context,
              we::runtime::renderer::ResourceManager* resources,
              VkFormat swapchainColorFormat);
    void Shutdown();

    void PrepareFrame(uint32_t width, uint32_t height, uint32_t frameIndex,
                      const std::shared_ptr<Widget>& root);

    void RecordDrawCommands(VkCommandBuffer cmd,
                            VkImageView targetView,
                            VkFormat targetFormat,
                            uint32_t width,
                            uint32_t height,
                            uint32_t frameIndex);

    void Render(VkCommandBuffer cmd,
                VkImageView targetView,
                VkFormat targetFormat,
                uint32_t width,
                uint32_t height,
                uint32_t frameIndex,
                const std::shared_ptr<Widget>& root);

    VkDescriptorSet RegisterTexture(VkImageView imageView, VkSampler sampler);
    void UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);
    void UnregisterTexture(VkDescriptorSet descSet);

    VkDescriptorSetLayout GetTextureLayout() const { return m_TextureDescLayout; }
    std::shared_ptr<FontAtlas> GetFontAtlas() const { return m_FontAtlas; }
    std::shared_ptr<IconRenderer> GetIconRenderer() const { return m_IconRenderer; }

    struct FrameStats {
        uint32_t drawCommands = 0;
        uint32_t vertices = 0;
        uint32_t indices = 0;
        uint32_t batches = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    const FrameStats& GetLastFrameStats() const { return m_LastFrameStats; }

private:
    void CreatePipeline(VkFormat colorFormat);
    void CreateDummyTexture();
    void BuildGeometry(const std::vector<DrawCommand>& commands, uint32_t width, uint32_t height);
    void UpdateBuffers(uint32_t frameIndex);

    struct FrameGeometryBuffers {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexBufferSize = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize indexBufferSize = 0;
    };

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    UiGpuUpload m_GpuUpload;
    VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;

    std::shared_ptr<FontAtlas> m_FontAtlas;
    std::shared_ptr<IconRenderer> m_IconRenderer;

    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    VkImage m_DummyImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DummyMemory = VK_NULL_HANDLE;
    VkImageView m_DummyImageView = VK_NULL_HANDLE;
    VkSampler m_DummySampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DummyDescriptorSet = VK_NULL_HANDLE;

    std::vector<UIVertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::array<FrameGeometryBuffers, we::runtime::renderer::kMaxFramesInFlight> m_FrameBuffers{};

    struct DrawBatch {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        VkDescriptorSet textureSet;
        Rect scissor;
    };
    std::vector<DrawBatch> m_Batches;

    FrameStats m_LastFrameStats{};
};

} // namespace we::UI
