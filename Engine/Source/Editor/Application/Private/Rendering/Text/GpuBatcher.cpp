#include "Rendering/Text/GpuBatcher.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cstring>
#include <functional>

namespace we::UI::Text {

// ============================================================================
// GpuBatcher Implementation
// ============================================================================

GpuBatcher::GpuBatcher() = default;

GpuBatcher::~GpuBatcher() {
    Shutdown();
}

bool GpuBatcher::Initialize(const GpuBatcherConfig& config) {
    if (m_Initialized) {
        HE_WARN("GpuBatcher: Already initialized");
        return true;
    }

    m_Config = config;
    m_Batches.clear();
    m_BatchMap.clear();
    m_VertexBufferCPU.clear();
    m_IndexBufferCPU.clear();
    m_Stats = {};
    m_Dirty = false;
    m_InFrame = false;

    // Reserve CPU buffers
    m_VertexBufferCPU.reserve(m_Config.maxVertices);
    m_IndexBufferCPU.reserve(m_Config.maxIndices);

    // Allocate GPU buffers (backend-specific - placeholder for now)
    if (!AllocateBuffers()) {
        HE_ERROR("GpuBatcher: Failed to allocate GPU buffers");
        return false;
    }

    m_Initialized = true;
    HE_INFO("GpuBatcher: Initialized with max " + std::to_string(m_Config.maxVertices) + 
            " vertices, " + std::to_string(m_Config.maxIndices) + " indices");
    return true;
}

void GpuBatcher::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    ClearBatches();
    FreeBuffers();
    
    m_VertexBufferCPU.clear();
    m_IndexBufferCPU.clear();
    m_Stats = {};
    m_Dirty = false;
    m_InFrame = false;
    m_Initialized = false;

    HE_INFO("GpuBatcher: Shutdown");
}

void GpuBatcher::BeginFrame() {
    if (!m_Initialized) {
        return;
    }

    m_InFrame = true;
    ClearBatches();
    m_VertexBufferCPU.clear();
    m_IndexBufferCPU.clear();
}

void GpuBatcher::EndFrame() {
    if (!m_Initialized || !m_InFrame) {
        return;
    }

    m_InFrame = false;

    if (m_Config.enableBatching) {
        SortBatches();
    }

    m_Dirty = true;
    UpdateStats();
}

bool GpuBatcher::AddGlyph(const GlyphQuad& quad) {
    if (!m_Initialized || !m_InFrame) {
        return false;
    }

    if (IsBatchFull()) {
        HE_WARN("GpuBatcher: Batch capacity reached");
        return false;
    }

    BatchKey key = {quad.atlasId, quad.shaderId, quad.materialId};
    GlyphBatch& batch = GetOrCreateBatch(key);

    // Add vertices
    uint32_t vertexOffset = static_cast<uint32_t>(m_VertexBufferCPU.size());
    for (int i = 0; i < 4; ++i) {
        m_VertexBufferCPU.push_back(quad.vertices[i]);
    }

    // Add indices
    uint32_t indexOffset = static_cast<uint32_t>(m_IndexBufferCPU.size());
    for (int i = 0; i < 6; ++i) {
        uint16_t index = static_cast<uint16_t>(kQuadIndices[i] + vertexOffset);
        m_IndexBufferCPU.push_back(index);
    }

    // Update batch offsets
    batch.vertexOffset = vertexOffset;
    batch.indexOffset = indexOffset;

    if (m_Config.trackStatistics) {
        m_Stats.totalVertices += 4;
        m_Stats.totalIndices += 6;
    }

    return true;
}

size_t GpuBatcher::AddGlyphs(const std::vector<GlyphQuad>& quads) {
    size_t added = 0;
    for (const auto& quad : quads) {
        if (AddGlyph(quad)) {
            added++;
        }
    }
    return added;
}

void GpuBatcher::BuildQuad(const AtlasGlyphPlacement& placement, float x, float y,
                           const float color[4], float edgeSoftness, uint32_t atlasId,
                           GlyphQuad& outQuad) {
    // Calculate quad positions
    float x0 = x + placement.planeLeft;
    float y0 = y - placement.planeTop;
    float x1 = x + placement.planeRight;
    float y1 = y - placement.planeBottom;

    // Build 4 vertices (counter-clockwise winding)
    // Vertex 0: Top-left
    outQuad.vertices[0].positionX = x0;
    outQuad.vertices[0].positionY = y0;
    outQuad.vertices[0].uvX = placement.atlasLeft;
    outQuad.vertices[0].uvY = placement.atlasTop;
    outQuad.vertices[0].colorR = color[0];
    outQuad.vertices[0].colorG = color[1];
    outQuad.vertices[0].colorB = color[2];
    outQuad.vertices[0].colorA = color[3];
    outQuad.vertices[0].paramX = edgeSoftness;
    outQuad.vertices[0].paramY = 0.0f;  // outline width
    outQuad.vertices[0].paramZ = 0.0f;  // outline softness
    outQuad.vertices[0].paramW = 0.0f;  // effect flags

    // Vertex 1: Top-right
    outQuad.vertices[1].positionX = x1;
    outQuad.vertices[1].positionY = y0;
    outQuad.vertices[1].uvX = placement.atlasRight;
    outQuad.vertices[1].uvY = placement.atlasTop;
    outQuad.vertices[1].colorR = color[0];
    outQuad.vertices[1].colorG = color[1];
    outQuad.vertices[1].colorB = color[2];
    outQuad.vertices[1].colorA = color[3];
    outQuad.vertices[1].paramX = edgeSoftness;
    outQuad.vertices[1].paramY = 0.0f;
    outQuad.vertices[1].paramZ = 0.0f;
    outQuad.vertices[1].paramW = 0.0f;

    // Vertex 2: Bottom-left
    outQuad.vertices[2].positionX = x0;
    outQuad.vertices[2].positionY = y1;
    outQuad.vertices[2].uvX = placement.atlasLeft;
    outQuad.vertices[2].uvY = placement.atlasBottom;
    outQuad.vertices[2].colorR = color[0];
    outQuad.vertices[2].colorG = color[1];
    outQuad.vertices[2].colorB = color[2];
    outQuad.vertices[2].colorA = color[3];
    outQuad.vertices[2].paramX = edgeSoftness;
    outQuad.vertices[2].paramY = 0.0f;
    outQuad.vertices[2].paramZ = 0.0f;
    outQuad.vertices[2].paramW = 0.0f;

    // Vertex 3: Bottom-right
    outQuad.vertices[3].positionX = x1;
    outQuad.vertices[3].positionY = y1;
    outQuad.vertices[3].uvX = placement.atlasRight;
    outQuad.vertices[3].uvY = placement.atlasBottom;
    outQuad.vertices[3].colorR = color[0];
    outQuad.vertices[3].colorG = color[1];
    outQuad.vertices[3].colorB = color[2];
    outQuad.vertices[3].colorA = color[3];
    outQuad.vertices[3].paramX = edgeSoftness;
    outQuad.vertices[3].paramY = 0.0f;
    outQuad.vertices[3].paramZ = 0.0f;
    outQuad.vertices[3].paramW = 0.0f;

    outQuad.atlasId = atlasId;
    outQuad.shaderId = 0;  // Default shader
    outQuad.materialId = 0;  // Default material
}

void GpuBatcher::ClearBatches() {
    m_Batches.clear();
    m_BatchMap.clear();
}

bool GpuBatcher::UpdateConfig(const GpuBatcherConfig& config) {
    m_Config = config;
    
    // Reallocate buffers if needed
    if (m_Config.maxVertices > m_VertexBufferCPU.capacity() ||
        m_Config.maxIndices > m_IndexBufferCPU.capacity()) {
        m_VertexBufferCPU.reserve(m_Config.maxVertices);
        m_IndexBufferCPU.reserve(m_Config.maxIndices);
        
        // Reallocate GPU buffers
        FreeBuffers();
        if (!AllocateBuffers()) {
            return false;
        }
    }
    
    return true;
}

GpuBatcherStats GpuBatcher::GetStatistics() const {
    GpuBatcherStats stats = m_Stats;
    stats.totalBatches = m_Batches.size();
    stats.memoryUsageBytes = m_VertexBufferCPU.size() * sizeof(TextVertex) +
                            m_IndexBufferCPU.size() * sizeof(uint16_t);
    
    // Calculate batching efficiency
    if (stats.totalBatches > 0) {
        float avgVerticesPerBatch = static_cast<float>(stats.totalVertices) / 
                                    static_cast<float>(stats.totalBatches);
        stats.batchingEfficiency = avgVerticesPerBatch / 4.0f;  // Normalize to quads
    }
    
    return stats;
}

void GpuBatcher::ResetStatistics() {
    m_Stats = {};
}

bool GpuBatcher::UploadBuffers() {
    if (!m_Dirty || m_VertexBufferCPU.empty()) {
        return true;
    }

    // Upload vertex buffer
    if (m_VertexBuffer.mappedPtr && m_Config.usePersistentMapping) {
        std::memcpy(m_VertexBuffer.mappedPtr, m_VertexBufferCPU.data(), 
                    m_VertexBufferCPU.size() * sizeof(TextVertex));
    } else {
        // Non-persistent mapping would require staging buffer
        // This is backend-specific implementation
        HE_WARN("GpuBatcher: Non-persistent mapping not yet implemented");
    }

    // Upload index buffer
    if (m_IndexBuffer.mappedPtr && m_Config.usePersistentMapping) {
        std::memcpy(m_IndexBuffer.mappedPtr, m_IndexBufferCPU.data(),
                    m_IndexBufferCPU.size() * sizeof(uint16_t));
    } else {
        HE_WARN("GpuBatcher: Non-persistent mapping not yet implemented");
    }

    m_Dirty = false;
    
    if (m_Config.trackStatistics) {
        m_Stats.bufferUploads++;
    }

    return true;
}

void GpuBatcher::SortBatches() {
    if (m_Config.enableSorting) {
        std::sort(m_Batches.begin(), m_Batches.end(),
                  [](const GlyphBatch& a, const GlyphBatch& b) {
                      return a.key < b.key;
                  });
    }
}

GlyphBatch& GpuBatcher::GetOrCreateBatch(const BatchKey& key) {
    auto it = m_BatchMap.find(key);
    if (it != m_BatchMap.end()) {
        return m_Batches[it->second];
    }

    // Create new batch
    GlyphBatch batch;
    batch.key = key;
    batch.vertexOffset = 0;
    batch.indexOffset = 0;
    
    size_t batchIndex = m_Batches.size();
    m_Batches.push_back(batch);
    m_BatchMap[key] = batchIndex;
    
    if (m_Batches.size() > m_Config.maxBatches) {
        HE_WARN("GpuBatcher: Exceeded maximum batch count");
    }
    
    return m_Batches.back();
}

bool GpuBatcher::IsBatchFull() const {
    return m_VertexBufferCPU.size() + 4 > m_Config.maxVertices ||
           m_IndexBufferCPU.size() + 6 > m_Config.maxIndices ||
           m_Batches.size() >= m_Config.maxBatches;
}

bool GpuBatcher::AllocateBuffers() {
    // This is a placeholder for backend-specific buffer allocation
    // In a real implementation, this would call the rendering backend
    // to create vertex and index buffers
    
    m_VertexBuffer.handle = 1;  // Placeholder handle
    m_VertexBuffer.size = static_cast<uint32_t>(m_Config.maxVertices * sizeof(TextVertex));
    m_VertexBuffer.mappedPtr = nullptr;  // Would be mapped in real implementation
    
    m_IndexBuffer.handle = 2;  // Placeholder handle
    m_IndexBuffer.size = static_cast<uint32_t>(m_Config.maxIndices * sizeof(uint16_t));
    m_IndexBuffer.mappedPtr = nullptr;  // Would be mapped in real implementation
    
    HE_INFO("GpuBatcher: Allocated GPU buffers (vertex: " + 
            std::to_string(m_VertexBuffer.size) + " bytes, index: " +
            std::to_string(m_IndexBuffer.size) + " bytes)");
    
    return true;
}

void GpuBatcher::FreeBuffers() {
    // This is a placeholder for backend-specific buffer deallocation
    m_VertexBuffer = {};
    m_IndexBuffer = {};
    
    HE_INFO("GpuBatcher: Freed GPU buffers");
}

void GpuBatcher::UpdateStats() {
    if (m_Config.trackStatistics) {
        m_Stats.totalBatches = m_Batches.size();
        m_Stats.totalVertices = m_VertexBufferCPU.size();
        m_Stats.totalIndices = m_IndexBufferCPU.size();
        m_Stats.totalDrawCalls = m_Batches.size();
    }
}

// ============================================================================
// GpuBatcherUtils Implementation
// ============================================================================

namespace GpuBatcherUtils {

size_t EstimateBatchMemory(const GlyphBatch& batch) {
    size_t vertexMemory = batch.vertices.size() * sizeof(TextVertex);
    size_t indexMemory = batch.indices.size() * sizeof(uint16_t);
    return vertexMemory + indexMemory;
}

GpuBatcherConfig GetDefaultConfig() {
    GpuBatcherConfig config;
    config.maxVertices = 65536;
    config.maxIndices = 65536;
    config.maxBatches = 1024;
    config.usePersistentMapping = true;
    config.enableBatching = true;
    config.enableSorting = true;
    config.trackStatistics = true;
    return config;
}

GpuBatcherConfig GetPerformanceConfig() {
    GpuBatcherConfig config = GetDefaultConfig();
    config.maxVertices = 262144;  // 4x default
    config.maxIndices = 262144;
    config.maxBatches = 4096;
    config.enableSorting = false;  // Skip sorting for performance
    return config;
}

GpuBatcherConfig GetMemoryEfficientConfig() {
    GpuBatcherConfig config = GetDefaultConfig();
    config.maxVertices = 16384;   // 4x smaller
    config.maxIndices = 16384;
    config.maxBatches = 256;
    config.usePersistentMapping = false;  // Use dynamic buffers
    return config;
}

bool ValidateVertex(const TextVertex& vertex, std::string& outError) {
    // Check position
    if (!std::isfinite(vertex.positionX) || !std::isfinite(vertex.positionY)) {
        outError = "Invalid position values";
        return false;
    }

    // Check UV coordinates
    if (vertex.uvX < 0.0f || vertex.uvX > 1.0f || vertex.uvY < 0.0f || vertex.uvY > 1.0f) {
        outError = "UV coordinates out of range [0, 1]";
        return false;
    }

    // Check color values
    if (vertex.colorR < 0.0f || vertex.colorR > 1.0f ||
        vertex.colorG < 0.0f || vertex.colorG > 1.0f ||
        vertex.colorB < 0.0f || vertex.colorB > 1.0f ||
        vertex.colorA < 0.0f || vertex.colorA > 1.0f) {
        outError = "Color values out of range [0, 1]";
        return false;
    }

    return true;
}

bool ValidateBatch(const GlyphBatch& batch, std::vector<std::string>& outErrors) {
    outErrors.clear();

    // Check vertex count (must be multiple of 4 for quads)
    if (batch.vertices.size() % 4 != 0) {
        outErrors.push_back("Vertex count is not a multiple of 4");
    }

    // Check index count (must be multiple of 6 for quads)
    if (batch.indices.size() % 6 != 0) {
        outErrors.push_back("Index count is not a multiple of 6");
    }

    // Check index count matches vertex count (6 indices per 4 vertices)
    size_t expectedIndices = (batch.vertices.size() / 4) * 6;
    if (batch.indices.size() != expectedIndices) {
        outErrors.push_back("Index count does not match vertex count");
    }

    // Validate individual vertices
    for (size_t i = 0; i < batch.vertices.size(); ++i) {
        std::string error;
        if (!ValidateVertex(batch.vertices[i], error)) {
            outErrors.push_back("Vertex " + std::to_string(i) + ": " + error);
        }
    }

    // Check index bounds
    uint16_t maxVertexIndex = static_cast<uint16_t>(batch.vertices.size() - 1);
    for (uint16_t index : batch.indices) {
        if (index > maxVertexIndex) {
            outErrors.push_back("Index out of bounds: " + std::to_string(index));
        }
    }

    return outErrors.empty();
}

} // namespace GpuBatcherUtils

} // namespace we::UI::Text
