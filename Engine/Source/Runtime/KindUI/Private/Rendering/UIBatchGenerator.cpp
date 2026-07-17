#include "WindEffects/Runtime/UI/Rendering/UIBatchGenerator.h"
#include "WindEffects/Runtime/UI/Rendering/OverlayRenderer.h"
#include <cmath>
#include <algorithm>

namespace WindEffects::Editor::UI {

UIBatchGenerator::UIBatchGenerator()
    : m_Width(0)
    , m_Height(0)
    , m_BatchingActive(false)
{
}

UIBatchGenerator::~UIBatchGenerator() {
    Shutdown();
}

void UIBatchGenerator::Initialize() {
    m_Batches.clear();
    m_Vertices.clear();
    m_Indices.clear();
    m_BatchingActive = false;
}

void UIBatchGenerator::Shutdown() {
    Clear();
}

void UIBatchGenerator::BeginBatching(uint32_t width, uint32_t height) {
    m_Width = width;
    m_Height = height;
    m_Batches.clear();
    m_Vertices.clear();
    m_Indices.clear();
    m_BatchingActive = true;
}

void UIBatchGenerator::AddGeometry(const std::vector<UIVertex2>& vertices,
                                   const std::vector<uint32_t>& indices,
                                   we::rhi::RHIDescriptorSetHandle textureSet,
                                   const UIDirtyRegion& scissor,
                                   uint32_t stencilRef) {
    if (!m_BatchingActive) {
        return;
    }
    
    uint32_t vertexOffset = static_cast<uint32_t>(m_Vertices.size());
    uint32_t indexOffset = static_cast<uint32_t>(m_Indices.size());
    
    // Add vertices
    m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
    
    // Add indices with offset
    for (uint32_t index : indices) {
        m_Indices.push_back(index + vertexOffset);
    }
    
    // Create batch
    UIRenderBatch batch;
    batch.textureSet = textureSet;
    batch.indexCount = static_cast<uint32_t>(indices.size());
    batch.firstIndex = indexOffset;
    batch.vertexOffset = static_cast<int32_t>(vertexOffset);
    batch.scissor[0] = static_cast<float>(scissor.x);
    batch.scissor[1] = static_cast<float>(scissor.y);
    batch.scissor[2] = static_cast<float>(scissor.width);
    batch.scissor[3] = static_cast<float>(scissor.height);
    batch.stencilRef = stencilRef;
    
    // Try to merge with previous batch
    if (!m_Batches.empty() && CanMergeBatches(m_Batches.back(), batch)) {
        m_Batches.back().indexCount += batch.indexCount;
    } else {
        m_Batches.push_back(batch);
    }
}

void UIBatchGenerator::EndBatching() {
    MergeBatches();
    m_BatchingActive = false;
}

void UIBatchGenerator::Clear() {
    m_Batches.clear();
    m_Vertices.clear();
    m_Indices.clear();
    m_BatchingActive = false;
}

void UIBatchGenerator::MergeBatches() {
    // Sort batches by texture set to improve merge opportunities
    std::sort(m_Batches.begin(), m_Batches.end(), 
        [](const UIRenderBatch& a, const UIRenderBatch& b) {
            return a.textureSet < b.textureSet;
        });
    
    // Merge compatible batches
    std::vector<UIRenderBatch> merged;
    for (const auto& batch : m_Batches) {
        if (!merged.empty() && CanMergeBatches(merged.back(), batch)) {
            merged.back().indexCount += batch.indexCount;
        } else {
            merged.push_back(batch);
        }
    }
    
    m_Batches = std::move(merged);
}

bool UIBatchGenerator::CanMergeBatches(const UIRenderBatch& a, const UIRenderBatch& b) const {
    // Must have same texture
    if (a.textureSet != b.textureSet) {
        return false;
    }
    
    // Must have same scissor
    if (std::abs(a.scissor[0] - b.scissor[0]) > 0.01f ||
        std::abs(a.scissor[1] - b.scissor[1]) > 0.01f ||
        std::abs(a.scissor[2] - b.scissor[2]) > 0.01f ||
        std::abs(a.scissor[3] - b.scissor[3]) > 0.01f) {
        return false;
    }
    
    // Must have same stencil reference
    if (a.stencilRef != b.stencilRef) {
        return false;
    }

    if (a.isText != b.isText) {
        return false;
    }

    if (a.isText
        && (a.atlasWidth != b.atlasWidth
            || a.atlasHeight != b.atlasHeight
            || std::abs(a.msdfPixelRange - b.msdfPixelRange) > 0.01f)) {
        return false;
    }
    
    return true;
}

} // namespace WindEffects::Editor::UI
