#pragma once

#include "Application/Export.h"

#include "Rendering/OverlayRenderer.h"
#include "Core/PaintContext.h"
#include "Core/Widget.h"
#include <memory>
#include <vector>

namespace we::UI {

// Adapter to bridge existing widget system with new OverlayRenderer
// Converts PaintContext/DrawCommand to OverlayRenderer format
class APPLICATION_API UIWidgetAdapter {
public:
    UIWidgetAdapter();
    ~UIWidgetAdapter();

    void Initialize(OverlayRenderer* renderer);
    void Shutdown();

    // Convert widget paint commands to renderer format
    void ProcessWidget(const std::shared_ptr<Widget>& root, 
                      uint32_t width, uint32_t height);

    // Get generated geometry for the renderer
    const std::vector<UIVertex2>& GetVertices() const { return m_Vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
    const std::vector<UIRenderBatch>& GetBatches() const { return m_Batches; }

    static uint32_t s_TotalDrawCommandsGenerated;
    static uint32_t s_RectangleCommands;
    static uint32_t s_TextCommands;
    static uint32_t s_ImageCommands;
    static uint32_t s_IconCommands;
    static uint32_t s_BorderCommands;
    static uint32_t s_GradientCommands;
    static uint32_t s_ShadowCommands;
    static uint32_t s_ClipRectCount;
    static uint32_t s_TextureSwitchCount;
    static uint32_t s_BatchCount;
    
    static void ResetDiagnostics();

private:
    void ConvertDrawCommand(const DrawCommand& cmd);
    void GenerateRectGeometry(const DrawCommand& cmd);
    void GenerateTextGeometry(const DrawCommand& cmd);
    void GenerateTextureGeometry(const DrawCommand& cmd);
    void GenerateIconGeometry(const DrawCommand& cmd);
    void GenerateLineGeometry(const DrawCommand& cmd);
    void GenerateShadowGeometry(const DrawCommand& cmd);
    void GenerateGradientGeometry(const DrawCommand& cmd);
    void GenerateRoundedOutlineGeometry(const DrawCommand& cmd);

    OverlayRenderer* m_Renderer;
    std::vector<UIVertex2> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<UIRenderBatch> m_Batches;
    
    uint32_t m_Width;
    uint32_t m_Height;
    VkDescriptorSet m_CurrentTextureSet;
    VkDescriptorSet m_DefaultTextureSet;
    UIDirtyRegion m_CurrentScissor;
};

} // namespace we::UI
