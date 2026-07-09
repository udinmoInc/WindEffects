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

    // Diagnostics (moved from static to instance-level to avoid global state)
    struct Diagnostics {
        uint32_t totalDrawCommandsGenerated = 0;
        uint32_t rectangleCommands = 0;
        uint32_t textCommands = 0;
        uint32_t imageCommands = 0;
        uint32_t iconCommands = 0;
        uint32_t borderCommands = 0;
        uint32_t gradientCommands = 0;
        uint32_t shadowCommands = 0;
        uint32_t clipRectCount = 0;
        uint32_t textureSwitchCount = 0;
        uint32_t batchCount = 0;
        uint32_t paintCommandsRecorded = 0;

        // Text pipeline diagnostics
        uint32_t textStringsProcessed = 0;
        uint32_t textGlyphsRequested = 0;
        uint32_t textGlyphsResolved = 0;
        uint32_t textGlyphsMissing = 0;
        uint32_t textVerticesGenerated = 0;
        uint32_t textIndicesGenerated = 0;
        uint32_t textBatchesCreated = 0;
        uint32_t textDrawsSkipped = 0;
        
        void Reset() {
            totalDrawCommandsGenerated = 0;
            rectangleCommands = 0;
            textCommands = 0;
            imageCommands = 0;
            iconCommands = 0;
            borderCommands = 0;
            gradientCommands = 0;
            shadowCommands = 0;
            clipRectCount = 0;
            textureSwitchCount = 0;
            batchCount = 0;
            paintCommandsRecorded = 0;
            textStringsProcessed = 0;
            textGlyphsRequested = 0;
            textGlyphsResolved = 0;
            textGlyphsMissing = 0;
            textVerticesGenerated = 0;
            textIndicesGenerated = 0;
            textBatchesCreated = 0;
            textDrawsSkipped = 0;
        }
    };
    
    // Get diagnostics for this adapter instance
    const Diagnostics& GetDiagnostics() const { return m_Diagnostics; }
    void ResetDiagnostics() { m_Diagnostics.Reset(); }
    
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
    Diagnostics m_Diagnostics;
};

} // namespace we::UI
