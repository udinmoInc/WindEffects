#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <volk.h>
#include <array>
#include <memory>
#include <vector>
#include <mutex>
#include <cstdint>

#include "Rendering/OverlayRenderContext.h"

namespace we::runtime::renderer {
    class DeviceContext;
    class ResourceManager;
}

#include "Rendering/UIStateManager.h"

namespace WindEffects::Editor::UI {

class Widget;
class UIWidgetAdapter;
class UICompositor;
class UIStateManager;
class IconRenderer;
class IconManager;
class TextUIService;
class UiGpuUpload;

struct UIFRAMEWORK_API UIVertex2 {
    float position[2];
    float uv[2];
    float color[4];
    float sdfRect[4];
    float sdfParams[4];
};

struct UIRenderBatch {
    VkDescriptorSet textureSet = VK_NULL_HANDLE;
    uint32_t indexCount = 0;
    uint32_t firstIndex = 0;
    int32_t vertexOffset = 0;
    float scissor[4]{};
    uint32_t stencilRef = 0;
    bool isText = false;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    float msdfPixelRange = 4.0f;
};

struct UIFrameStats {
    uint32_t drawCalls = 0;
    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t batches = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct UIDirtyRegion {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};



class UIFRAMEWORK_API OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    bool Init(VkPhysicalDevice physicalDevice,
              VkDevice logicalDevice,
              VkQueue graphicsQueue,
              uint32_t graphicsQueueFamilyIndex,
              VkFormat swapchainFormat,
              uint32_t maxFramesInFlight,
              ::we::runtime::renderer::DeviceContext* deviceContext,
              ::we::runtime::renderer::ResourceManager* resourceManager);
    void Shutdown();

    void BeginOverlayPass(const we::editor::rendering::OverlayRenderContext& context);
    void SetTargetExtent(uint32_t width, uint32_t height);
    void RenderEditorUI(const std::shared_ptr<WindEffects::Editor::UI::Widget>& root, uint32_t frameSlot);
    void EndOverlayPass(const we::editor::rendering::OverlayRenderContext& context);
    void SetPipelineAuditImageIndex(uint32_t imageIndex);

    VkDescriptorSet RegisterTexture(VkImageView imageView, VkSampler sampler);
    void UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);
    void UnregisterTexture(VkDescriptorSet descriptorSet);

    TextUIService* GetTextUIService() const;
    IconRenderer* GetIconRenderer() const;
    IconManager* GetIconManager() const;
    ::we::runtime::renderer::DeviceContext* GetDeviceContext() const { return m_DeviceContext; }
    ::we::runtime::renderer::ResourceManager* GetResourceManager() const { return m_ResourceManager; }
    UiGpuUpload* GetGpuUpload() const { return m_GpuUpload.get(); }
    VkDescriptorSet GetDummyDescriptorSet() const { return m_DummyDescriptorSet; }
    VkSampler GetDefaultSampler() const { return m_DummySampler; }

    const UIFrameStats& GetFrameStats() const { return m_FrameStats; }

private:
#pragma warning(push)
#pragma warning(disable: 4251)
    void CreateGraphicsPipeline(VkFormat colorFormat);
    void CreateTextGraphicsPipeline(VkFormat colorFormat);
    void CreateDescriptorSetLayouts();
    void CreateDescriptorPool();
    void CreateDummyTexture();
    void UpdateGeometryBuffers(uint32_t frameIndex);

    std::unique_ptr<UIWidgetAdapter> m_WidgetAdapter;
    std::unique_ptr<UICompositor> m_Compositor;
    std::unique_ptr<UIStateManager> m_StateManager;
    std::unique_ptr<UiGpuUpload> m_GpuUpload;

    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamilyIndex = 0;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    uint32_t m_MaxFramesInFlight = 2;

    ::we::runtime::renderer::DeviceContext* m_DeviceContext = nullptr;
    ::we::runtime::renderer::ResourceManager* m_ResourceManager = nullptr;

    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VkPipeline m_TextGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout m_TextPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    VkImage m_DummyImage = VK_NULL_HANDLE;
    VkDeviceMemory m_DummyMemory = VK_NULL_HANDLE;
    VkImageView m_DummyImageView = VK_NULL_HANDLE;
    VkSampler m_DummySampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DummyDescriptorSet = VK_NULL_HANDLE;

    struct FrameGeometry {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexSize = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize indexSize = 0;
        uint64_t geometryGeneration = 0;
    };
    std::vector<FrameGeometry> m_FrameGeometry;

    std::unique_ptr<TextUIService> m_TextUIService;
    std::unique_ptr<IconManager> m_IconManager;
    std::unique_ptr<IconRenderer> m_IconRenderer;

    std::vector<UIVertex2> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<UIRenderBatch> m_Batches;
    uint64_t m_GeometryGeneration = 0;

    uint32_t m_ActiveFrameSlot = 0;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    uint32_t m_LastBuiltWidth = 0;
    uint32_t m_LastBuiltHeight = 0;
    uint32_t m_PipelineAuditImageIndex = UINT32_MAX;
    UIFrameStats m_FrameStats;
    float m_LastFontBakeHeightPx = 0.0f;

    WindEffects::Editor::UI::SavedVulkanState m_SavedState;

    std::mutex m_Mutex;
#pragma warning(pop)
};

} // namespace WindEffects::Editor::UI
