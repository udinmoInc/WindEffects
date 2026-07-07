#pragma once

#include "Application/Export.h"

#include <volk.h>
#include <array>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace we::runtime::renderer {
    class DeviceContext;
    class ResourceManager;
}

namespace we::UI {

class Widget;
class PaintContext;

// Forward declarations for internal components
class UIGeometryCache;
class UIBatchGenerator;
class UITextureAtlasManager;
class UICommandBuffer;
class UICompositor;
class UIStateManager;

// Vertex format for UI rendering (UE5 Slate-inspired)
struct APPLICATION_API UIVertex2 {
    float position[2];      // Screen position
    float uv[2];           // Texture coordinates
    float color[4];         // RGBA color
    float sdfRect[4];       // SDF rectangle bounds
    float sdfParams[4];     // SDF parameters (radius, type, thickness, unused)
};

// Draw command types
enum class UIDrawCommandType : uint32_t {
    Rect,
    Text,
    Texture,
    Icon,
    Gradient,
    Shadow,
    RoundedOutline,
    Line
};

// Cached geometry key
struct UIGeometryKey {
    uint64_t widgetId;
    uint32_t geometryVersion;
    UIDrawCommandType type;
    
    bool operator==(const UIGeometryKey& other) const {
        return widgetId == other.widgetId && 
               geometryVersion == other.geometryVersion &&
               type == other.type;
    }
};

// Hash function for geometry keys
struct UIGeometryKeyHash {
    size_t operator()(const UIGeometryKey& key) const {
        size_t h1 = std::hash<uint64_t>{}(key.widgetId);
        size_t h2 = std::hash<uint32_t>{}(key.geometryVersion);
        size_t h3 = std::hash<uint32_t>{}(static_cast<uint32_t>(key.type));
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Render batch for efficient drawing
struct UIRenderBatch {
    VkDescriptorSet textureSet;
    uint32_t indexCount;
    uint32_t firstIndex;
    int32_t vertexOffset;
    float scissor[4]; // x, y, width, height
    uint32_t stencilRef;
};

// Frame statistics
struct UIFrameStats {
    uint32_t drawCalls = 0;
    uint32_t vertices = 0;
    uint32_t indices = 0;
    uint32_t batches = 0;
    uint32_t cacheHits = 0;
    uint32_t cacheMisses = 0;
    uint32_t dirtyRegions = 0;
    float cpuTimeMs = 0.0f;
};

// Dirty region for partial updates
struct UIDirtyRegion {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
};

// Main UE5-inspired UI Renderer
class APPLICATION_API UIRenderer2 {
public:
    UIRenderer2();
    ~UIRenderer2();

    // Initialization (completely independent from scene renderer)
    bool Init(VkPhysicalDevice physicalDevice,
              VkDevice logicalDevice,
              VkQueue graphicsQueue,
              uint32_t graphicsQueueFamilyIndex,
              VkFormat swapchainFormat,
              uint32_t maxFramesInFlight,
              we::runtime::renderer::DeviceContext* deviceContext,
              we::runtime::renderer::ResourceManager* resourceManager);
    void Shutdown();

    // Frame rendering
    void BeginFrame(uint32_t frameIndex, uint32_t width, uint32_t height);
    void RenderWidget(const std::shared_ptr<Widget>& root);
    void EndFrame(VkCommandBuffer cmd, VkImageView swapchainImageView);

    // Texture management (owned by UI renderer)
    VkDescriptorSet RegisterTexture(VkImageView imageView, VkSampler sampler);
    void UpdateTexture(VkDescriptorSet descriptorSet, VkImageView imageView, VkSampler sampler);
    void UnregisterTexture(VkDescriptorSet descriptorSet);

    // Font atlas access
    class FontAtlas* GetFontAtlas() const;
    class IconRenderer* GetIconRenderer() const;

    // Statistics
    const UIFrameStats& GetFrameStats() const { return m_FrameStats; }
    void ResetStats();

    // Dirty region management
    void InvalidateRegion(const UIDirtyRegion& region);
    void InvalidateAll();

private:
    // Vulkan state management (save/restore)
    void SaveVulkanState(VkCommandBuffer cmd);
    void RestoreVulkanState(VkCommandBuffer cmd);

    // Pipeline creation (completely independent)
    void CreateGraphicsPipeline(VkFormat colorFormat);
    void CreateDescriptorSetLayouts();
    void CreatePipelineLayout();
    void CreateDescriptorPool();

    // Buffer management
    void CreateGeometryBuffers(uint32_t frameIndex);
    void UpdateGeometryBuffers(uint32_t frameIndex);

    // Internal components
    std::unique_ptr<UIGeometryCache> m_GeometryCache;
    std::unique_ptr<UIBatchGenerator> m_BatchGenerator;
    std::unique_ptr<UITextureAtlasManager> m_TextureAtlasManager;
    std::unique_ptr<UICommandBuffer> m_CommandBuffer;
    std::unique_ptr<UICompositor> m_Compositor;
    std::unique_ptr<UIStateManager> m_StateManager;

    // Vulkan resources (owned by UI renderer)
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_LogicalDevice = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamilyIndex = 0;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    uint32_t m_MaxFramesInFlight = 2;

    // External dependencies
    we::runtime::renderer::DeviceContext* m_DeviceContext = nullptr;
    we::runtime::renderer::ResourceManager* m_ResourceManager = nullptr;

    // Graphics pipeline (completely independent)
    VkPipeline m_GraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_TextureDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

    // Geometry buffers (per-frame)
    struct FrameGeometry {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexSize = 0;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize indexSize = 0;
    };
    std::vector<FrameGeometry> m_FrameGeometry;

    // Font and icon rendering
    std::unique_ptr<class FontAtlas> m_FontAtlas;
    std::unique_ptr<class IconRenderer> m_IconRenderer;

    // Frame state
    uint32_t m_CurrentFrameIndex = 0;
    uint32_t m_CurrentWidth = 0;
    uint32_t m_CurrentHeight = 0;
    UIFrameStats m_FrameStats;

    // Dirty regions
    std::vector<UIDirtyRegion> m_DirtyRegions;
    bool m_FullRedrawNeeded = true;

    // Mutex for thread safety
    std::mutex m_Mutex;
};

} // namespace we::UI
