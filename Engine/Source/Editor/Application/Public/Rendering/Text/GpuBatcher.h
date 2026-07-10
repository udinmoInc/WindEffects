#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/MsdfAtlasGenerator.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace we::UI::Text {

/**
 * @brief Vertex format for text rendering
 */
struct TextVertex {
    float positionX;      // Screen X position
    float positionY;      // Screen Y position
    float uvX;            // Atlas U coordinate
    float uvY;            // Atlas V coordinate
    float colorR;         // Color red channel
    float colorG;         // Color green channel
    float colorB;         // Color blue channel
    float colorA;         // Color alpha channel
    float paramX;         // Parameter X (edge softness)
    float paramY;         // Parameter Y (outline width)
    float paramZ;         // Parameter Z (outline softness)
    float paramW;         // Parameter W (effect flags)
};

/**
 * @brief Index for quad rendering (6 indices per quad)
 */
static constexpr uint16_t kQuadIndices[] = {
    0, 1, 2,  // First triangle
    2, 1, 3   // Second triangle
};

/**
 * @brief Batch key for sorting glyphs
 */
struct BatchKey {
    uint32_t atlasId;      // Atlas texture ID
    uint32_t shaderId;     // Shader permutation ID
    uint32_t materialId;   // Material ID (for effects)
    
    bool operator==(const BatchKey& other) const {
        return atlasId == other.atlasId && 
               shaderId == other.shaderId && 
               materialId == other.materialId;
    }
    
    bool operator<(const BatchKey& other) const {
        if (atlasId != other.atlasId) return atlasId < other.atlasId;
        if (shaderId != other.shaderId) return shaderId < other.shaderId;
        return materialId < other.materialId;
    }
};

/**
 * @brief Hash function for BatchKey (for unordered_map)
 */
struct BatchKeyHash {
    size_t operator()(const BatchKey& key) const noexcept {
        size_t h1 = std::hash<uint32_t>{}(key.atlasId);
        size_t h2 = std::hash<uint32_t>{}(key.shaderId);
        size_t h3 = std::hash<uint32_t>{}(key.materialId);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief Glyph quad for rendering
 */
struct GlyphQuad {
    TextVertex vertices[4];  // 4 vertices per quad
    uint32_t atlasId;
    uint32_t shaderId;
    uint32_t materialId;
};

/**
 * @brief Batch of glyphs sharing the same atlas
 */
struct GlyphBatch {
    std::vector<TextVertex> vertices;
    std::vector<uint16_t> indices;
    BatchKey key;
    uint32_t vertexOffset;
    uint32_t indexOffset;
};

/**
 * @brief GPU buffer handle (backend-agnostic)
 */
struct GpuBufferHandle {
    uint64_t handle = 0;
    uint32_t size = 0;
    void* mappedPtr = nullptr;
};

/**
 * @brief GPU batcher statistics
 */
struct GpuBatcherStats {
    size_t totalBatches = 0;
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t totalDrawCalls = 0;
    size_t bufferUploads = 0;
    size_t memoryUsageBytes = 0;
    float batchingEfficiency = 0.0f;
};

/**
 * @brief GPU batcher configuration
 */
struct GpuBatcherConfig {
    size_t maxVertices = 65536;        // Maximum vertices per frame
    size_t maxIndices = 65536;         // Maximum indices per frame
    size_t maxBatches = 1024;          // Maximum batches per frame
    bool usePersistentMapping = true;  // Use persistent mapped buffers
    bool enableBatching = true;        // Enable batching by atlas
    bool enableSorting = true;         // Sort batches for efficiency
    bool trackStatistics = true;       // Track performance statistics
};

/**
 * @brief GPU batcher interface
 * 
 * Batches glyphs for efficient GPU rendering using persistent mapped vertex buffers.
 * Minimizes draw calls by grouping glyphs sharing the same atlas texture.
 */
class APPLICATION_API IGpuBatcher {
public:
    virtual ~IGpuBatcher() = default;

    /**
     * @brief Initialize the GPU batcher
     * @param config Configuration options
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const GpuBatcherConfig& config) = 0;

    /**
     * @brief Shutdown the GPU batcher
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Begin a new frame
     */
    virtual void BeginFrame() = 0;

    /**
     * @brief End the current frame and prepare for rendering
     */
    virtual void EndFrame() = 0;

    /**
     * @brief Add a glyph quad to the batch
     * @param quad Glyph quad to add
     * @return true if added, false if batch is full
     */
    virtual bool AddGlyph(const GlyphQuad& quad) = 0;

    /**
     * @brief Add multiple glyph quads to the batch
     * @param quads Vector of glyph quads
     * @return Number of quads added
     */
    virtual size_t AddGlyphs(const std::vector<GlyphQuad>& quads) = 0;

    /**
     * @brief Build glyph quad from placement and color
     * @param placement Atlas glyph placement
     * @param x Screen X position
     * @param y Screen Y position
     * @param color RGBA color
     * @param edgeSoftness Edge softness parameter
     * @param atlasId Atlas texture ID
     * @param outQuad Output glyph quad
     */
    virtual void BuildQuad(const AtlasGlyphPlacement& placement, float x, float y,
                           const float color[4], float edgeSoftness, uint32_t atlasId,
                           GlyphQuad& outQuad) = 0;

    /**
     * @brief Get all batches for rendering
     * @return Vector of glyph batches
     */
    virtual const std::vector<GlyphBatch>& GetBatches() const = 0;

    /**
     * @brief Clear all batches
     */
    virtual void ClearBatches() = 0;

    /**
     * @brief Get current configuration
     * @return GpuBatcherConfig structure
     */
    virtual GpuBatcherConfig GetConfig() const = 0;

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    virtual bool UpdateConfig(const GpuBatcherConfig& config) = 0;

    /**
     * @brief Get batcher statistics
     * @return GpuBatcherStats structure
     */
    virtual GpuBatcherStats GetStatistics() const = 0;

    /**
     * @brief Reset statistics
     */
    virtual void ResetStatistics() = 0;

    /**
     * @brief Get vertex buffer handle (for rendering)
     * @return GpuBufferHandle
     */
    virtual GpuBufferHandle GetVertexBuffer() const = 0;

    /**
     * @brief Get index buffer handle (for rendering)
     * @return GpuBufferHandle
     */
    virtual GpuBufferHandle GetIndexBuffer() const = 0;

    /**
     * @brief Check if buffers need to be uploaded to GPU
     * @return true if dirty, false otherwise
     */
    virtual bool IsDirty() const = 0;

    /**
     * @brief Clear dirty flag
     */
    virtual void ClearDirty() = 0;

    /**
     * @brief Upload buffers to GPU (backend-specific)
     * @return true if successful, false otherwise
     */
    virtual bool UploadBuffers() = 0;
};

/**
 * @brief Standard GPU batcher implementation
 */
class APPLICATION_API GpuBatcher : public IGpuBatcher {
public:
    GpuBatcher();
    ~GpuBatcher() override;

    // Disable copying
    GpuBatcher(const GpuBatcher&) = delete;
    GpuBatcher& operator=(const GpuBatcher&) = delete;

    bool Initialize(const GpuBatcherConfig& config) override;
    void Shutdown() override;

    void BeginFrame() override;
    void EndFrame() override;

    bool AddGlyph(const GlyphQuad& quad) override;
    size_t AddGlyphs(const std::vector<GlyphQuad>& quads) override;

    void BuildQuad(const AtlasGlyphPlacement& placement, float x, float y,
                   const float color[4], float edgeSoftness, uint32_t atlasId,
                   GlyphQuad& outQuad) override;

    const std::vector<GlyphBatch>& GetBatches() const override { return m_Batches; }
    void ClearBatches() override;

    GpuBatcherConfig GetConfig() const override { return m_Config; }
    bool UpdateConfig(const GpuBatcherConfig& config) override;

    GpuBatcherStats GetStatistics() const override;
    void ResetStatistics() override;

    GpuBufferHandle GetVertexBuffer() const override { return m_VertexBuffer; }
    GpuBufferHandle GetIndexBuffer() const override { return m_IndexBuffer; }

    bool IsDirty() const override { return m_Dirty; }
    void ClearDirty() override { m_Dirty = false; }

    bool UploadBuffers() override;

private:
    /**
     * @brief Sort glyphs into batches by atlas
     */
    void SortBatches();

    /**
     * @brief Create or get batch for a key
     * @param key Batch key
     * @return Reference to batch
     */
    GlyphBatch& GetOrCreateBatch(const BatchKey& key);

    /**
     * @brief Check if batch capacity is reached
     * @return true if full, false otherwise
     */
    bool IsBatchFull() const;

    /**
     * @brief Allocate vertex and index buffers
     * @return true if successful, false otherwise
     */
    bool AllocateBuffers();

    /**
     * @brief Free vertex and index buffers
     */
    void FreeBuffers();

    /**
     * @brief Update statistics
     */
    void UpdateStats();

    GpuBatcherConfig m_Config;
    
    std::vector<GlyphBatch> m_Batches;
    std::unordered_map<BatchKey, size_t, BatchKeyHash> m_BatchMap;
    
    std::vector<TextVertex> m_VertexBufferCPU;
    std::vector<uint16_t> m_IndexBufferCPU;
    
    GpuBufferHandle m_VertexBuffer;
    GpuBufferHandle m_IndexBuffer;
    
    GpuBatcherStats m_Stats;
    bool m_Dirty = false;
    bool m_InFrame = false;
    bool m_Initialized = false;
};

/**
 * @brief GPU batcher utility functions
 */
namespace GpuBatcherUtils {
    /**
     * @brief Calculate vertex count for a number of glyphs
     */
    inline constexpr size_t CalculateVertexCount(size_t glyphCount) {
        return glyphCount * 4;
    }

    /**
     * @brief Calculate index count for a number of glyphs
     */
    inline constexpr size_t CalculateIndexCount(size_t glyphCount) {
        return glyphCount * 6;
    }

    /**
     * @brief Estimate memory usage for a batch
     */
    size_t EstimateBatchMemory(const GlyphBatch& batch);

    /**
     * @brief Get default batcher configuration
     */
    GpuBatcherConfig GetDefaultConfig();

    /**
     * @brief Get high-performance batcher configuration
     */
    GpuBatcherConfig GetPerformanceConfig();

    /**
     * @brief Get memory-efficient batcher configuration
     */
    GpuBatcherConfig GetMemoryEfficientConfig();

    /**
     * @brief Validate vertex data
     */
    bool ValidateVertex(const TextVertex& vertex, std::string& outError);

    /**
     * @brief Validate batch data
     */
    bool ValidateBatch(const GlyphBatch& batch, std::vector<std::string>& outErrors);
};

} // namespace we::UI::Text
