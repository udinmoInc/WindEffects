#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <volk.h>
#include <vector>
#include <memory>

namespace WindEffects::Editor::UI {

// Forward declarations
struct UIRenderBatch;
struct UIVertex2;

// UI command buffer for recording draw commands (UE5 Slate-inspired)
class UIFRAMEWORK_API UICommandBuffer {
public:
    UICommandBuffer();
    ~UICommandBuffer();

    void Initialize(VkDevice device, uint32_t maxFramesInFlight);
    void Shutdown();

    // Command recording
    void BeginRecording(uint32_t frameIndex);
    void RecordDraw(const UIRenderBatch& batch,
                    const std::vector<UIVertex2>& vertices,
                    const std::vector<uint32_t>& indices);
    void EndRecording();

    // Execution
    void Execute(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout layout);

    // Buffer access
    VkBuffer GetVertexBuffer(uint32_t frameIndex) const;
    VkBuffer GetIndexBuffer(uint32_t frameIndex) const;
    VkDeviceSize GetVertexOffset(uint32_t frameIndex) const;
    VkDeviceSize GetIndexOffset(uint32_t frameIndex) const;

    // Statistics
    uint32_t GetDrawCallCount() const { return m_DrawCallCount; }

private:
    void CreateCommandPool();
    void CreateGeometryBuffers();
    void UpdateGeometryBuffers();

    VkDevice m_Device;
    uint32_t m_MaxFramesInFlight;

    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    
    struct FrameData {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        VkDeviceSize vertexOffset = 0;
        VkDeviceSize indexOffset = 0;
        std::vector<UIVertex2> vertexStaging;
        std::vector<uint32_t> indexStaging;
    };
    std::vector<FrameData> m_FrameData;

    uint32_t m_CurrentFrameIndex;
    uint32_t m_DrawCallCount;
    bool m_Recording;
};

} // namespace WindEffects::Editor::UI
