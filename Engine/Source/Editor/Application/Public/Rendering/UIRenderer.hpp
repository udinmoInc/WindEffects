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

#include "Renderer/VulkanContext.hpp"

namespace we::UI {

class Widget;

struct APPLICATION_API UIVertex {
    float pos[2];
    float uv[2];
    float color[4];
    float sdfRect[4];   // x, y, width, height (of the primitive bounds)
    float sdfParams[4]; // x=cornerRadius, y=type (0=texture, 1=sdf-rect)
};

class APPLICATION_API UIRenderer {
public:
    UIRenderer() = default;
    ~UIRenderer();

    bool Init(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, VkRenderPass renderPass);
    void Shutdown();

    // Render the UI widget tree. frameIndex must match Renderer::GetCurrentFrameIndex().
    void Render(VkCommandBuffer cmd, uint32_t width, uint32_t height, uint32_t frameIndex,
                const std::shared_ptr<Widget>& root);

    // Get descriptor set for an external texture (like the viewport offscreen buffer)
    VkDescriptorSet RegisterTexture(VkImageView imageView, VkSampler sampler);
    void UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);
    void UnregisterTexture(VkDescriptorSet descSet);

    // Helpers
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
    void CreatePipeline(VkRenderPass renderPass);
    void CreateDummyTexture();
    void BuildGeometry(const std::vector<DrawCommand>& commands, uint32_t width, uint32_t height);
    void UpdateBuffers(uint32_t frameIndex);

    static constexpr uint32_t kFramesInFlight = 2;

    struct FrameGeometryBuffers {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexBufferSize = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize indexBufferSize = 0;
    };

    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    std::shared_ptr<FontAtlas> m_FontAtlas;
    std::shared_ptr<IconRenderer> m_IconRenderer;
    // Pipeline resources
    VkDescriptorSetLayout m_TextureDescLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;

    // Dummy white texture (for untextured drawings)
    VkImage m_DummyImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DummyMemory = VK_NULL_HANDLE;
    VkImageView m_DummyImageView = VK_NULL_HANDLE;
    VkSampler m_DummySampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DummyDescriptorSet = VK_NULL_HANDLE;

    // Geometry buffers
    std::vector<UIVertex> m_Vertices;
    std::vector<uint32_t> m_Indices;

    std::array<FrameGeometryBuffers, kFramesInFlight> m_FrameBuffers{};

    // Drawing batches
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

} // namespace we::editor::application::UI
