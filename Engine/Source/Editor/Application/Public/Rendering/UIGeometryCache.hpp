#pragma once

#include "Application/Export.hpp"
#include "Rendering/UIRenderer2.hpp"

#include <volk.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace we::UI {

// Cached geometry entry
struct APPLICATION_API UIGeometryEntry {
    std::vector<UIVertex2> vertices;
    std::vector<uint32_t> indices;
    uint32_t lastUsedFrame;
    bool isDirty;
};

// Geometry cache for retained-mode rendering (UE5 Slate-inspired)
class APPLICATION_API UIGeometryCache {
public:
    UIGeometryCache();
    ~UIGeometryCache();

    void Initialize(uint32_t maxCacheSizeMB);
    void Shutdown();

    // Cache operations
    bool GetGeometry(const UIGeometryKey& key, 
                    std::vector<UIVertex2>& outVertices,
                    std::vector<uint32_t>& outIndices);
    
    void CacheGeometry(const UIGeometryKey& key,
                       const std::vector<UIVertex2>& vertices,
                       const std::vector<uint32_t>& indices,
                       uint32_t frameIndex);

    void InvalidateGeometry(const UIGeometryKey& key);
    void InvalidateWidget(uint64_t widgetId);
    void InvalidateAll();

    // Cache statistics
    size_t GetCacheSizeBytes() const;
    size_t GetCacheEntryCount() const;
    uint32_t GetCacheHitCount() const { return m_CacheHits; }
    uint32_t GetCacheMissCount() const { return m_CacheMisses; }
    void ResetStats();

    // Memory management
    void EvictOldEntries(uint32_t currentFrame, uint32_t maxAge);
    void CompactCache();

private:
    void EvictLRU();

    std::unordered_map<UIGeometryKey, UIGeometryEntry, UIGeometryKeyHash> m_Cache;
    mutable std::mutex m_Mutex;

    size_t m_MaxCacheSizeBytes;
    size_t m_CurrentCacheSizeBytes;
    uint32_t m_CacheHits;
    uint32_t m_CacheMisses;
};

} // namespace we::UI
