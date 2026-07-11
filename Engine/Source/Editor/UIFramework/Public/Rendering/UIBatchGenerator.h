#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <volk.h>
#include <vector>
#include <memory>

namespace WindEffects::Editor::UI {

// Forward declarations
struct UIVertex2;
struct UIRenderBatch;
struct UIDirtyRegion;

// Batch generator for efficient rendering (UE5 Slate-inspired)
class UIFRAMEWORK_API UIBatchGenerator {
public:
    UIBatchGenerator();
    ~UIBatchGenerator();

    void Initialize();
    void Shutdown();

    // Batch generation
    void BeginBatching(uint32_t width, uint32_t height);
    void AddGeometry(const std::vector<UIVertex2>& vertices,
                     const std::vector<uint32_t>& indices,
                     VkDescriptorSet textureSet,
                     const UIDirtyRegion& scissor,
                     uint32_t stencilRef = 0);
    void EndBatching();

    // Batch access
    const std::vector<UIRenderBatch>& GetBatches() const { return m_Batches; }
    const std::vector<UIVertex2>& GetVertices() const { return m_Vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

    // Statistics
    uint32_t GetBatchCount() const { return static_cast<uint32_t>(m_Batches.size()); }
    uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_Vertices.size()); }
    uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_Indices.size()); }

    void Clear();

private:
    void MergeBatches();
    bool CanMergeBatches(const UIRenderBatch& a, const UIRenderBatch& b) const;

    std::vector<UIRenderBatch> m_Batches;
    std::vector<UIVertex2> m_Vertices;
    std::vector<uint32_t> m_Indices;

    uint32_t m_Width;
    uint32_t m_Height;
    bool m_BatchingActive;
};

} // namespace WindEffects::Editor::UI
