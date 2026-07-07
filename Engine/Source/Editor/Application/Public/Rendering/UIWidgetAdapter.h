#pragma once

#include "Application/Export.h"

#include "Rendering/UIRenderer2.h"
#include "Core/PaintContext.h"
#include "Core/Widget.h"
#include <memory>
#include <vector>

namespace we::UI {

// Adapter to bridge existing widget system with new UIRenderer2
// Converts PaintContext/DrawCommand to UIRenderer2 format
class APPLICATION_API UIWidgetAdapter {
public:
    UIWidgetAdapter();
    ~UIWidgetAdapter();

    void Initialize(UIRenderer2* renderer);
    void Shutdown();

    // Convert widget paint commands to renderer format
    void ProcessWidget(const std::shared_ptr<Widget>& root, 
                      uint32_t width, uint32_t height);

    // Get generated geometry for the renderer
    const std::vector<UIVertex2>& GetVertices() const { return m_Vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
    const std::vector<UIRenderBatch>& GetBatches() const { return m_Batches; }

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

    UIRenderer2* m_Renderer;
    std::vector<UIVertex2> m_Vertices;
    std::vector<uint32_t> m_Indices;
    std::vector<UIRenderBatch> m_Batches;
    
    uint32_t m_Width;
    uint32_t m_Height;
    VkDescriptorSet m_CurrentTextureSet;
    UIDirtyRegion m_CurrentScissor;
};

} // namespace we::UI
