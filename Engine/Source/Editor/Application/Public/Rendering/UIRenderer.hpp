#pragma once

#include <volk.h>
#include <memory>
#include <vector>
#include "../Core/Geometry.hpp"
#include "../Core/PaintContext.hpp"
#include "FontAtlas.hpp"

namespace we::editor::application {
class VulkanContext;
}

namespace we::UI {

class Widget;

struct UIVertex {
    float pos[2];
    float uv[2];
    float color[4];
    float sdfRect[4];   // x, y, width, height (of the primitive bounds)
    float sdfParams[4]; // x=cornerRadius, y=type (0=texture, 1=sdf-rect)
};

class UIRenderer {
public:
    UIRenderer() = default;
    ~UIRenderer();

    bool Init(const std::shared_ptr<VulkanContext>& context, VkRenderPass renderPass);
    void Shutdown();

    // Render the UI widget tree
    void Render(VkCommandBuffer cmd, uint32_t width, uint32_t height, const std::shared_ptr<Widget>& root);

    // Get descriptor set for an external texture (like the viewport offscreen buffer)
    VkDescriptorSet RegisterTexture(VkImageView imageView, VkSampler sampler);
    void UnregisterTexture(VkDescriptorSet descSet);

    // Helpers
    VkDescriptorSetLayout GetTextureLayout() const { return m_TextureDescLayout; }
    std::shared_ptr<FontAtlas> GetFontAtlas() const { return m_FontAtlas; }

private:
    void CreatePipeline(VkRenderPass renderPass);
    void CreateDummyTexture();
    void BuildGeometry(const std::vector<DrawCommand>& commands, uint32_t width, uint32_t height);
    void UpdateBuffers();

    std::shared_ptr<VulkanContext> m_Context;
    std::shared_ptr<FontAtlas> m_FontAtlas;
    std::shared_ptr<FontAtlas> m_IconAtlas;
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

    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexMemory = VK_NULL_HANDLE;
    VkDeviceSize m_VertexBufferSize = 0;

    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexMemory = VK_NULL_HANDLE;
    VkDeviceSize m_IndexBufferSize = 0;

    // Drawing batches
    struct DrawBatch {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        VkDescriptorSet textureSet;
        Rect scissor;
    };
    std::vector<DrawBatch> m_Batches;
};

} // namespace we::editor::application::UI
