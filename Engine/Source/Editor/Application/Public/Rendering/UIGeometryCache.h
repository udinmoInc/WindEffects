#pragma once

#include "Application/Export.h"
#include "Rendering/OverlayRenderer.h"

#include <volk.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>

namespace we::UI {

// Geometry cache key for identifying cached geometry
struct APPLICATION_API UIGeometryKey {
    uint64_t widgetId;
    uint32_t styleHash;
    uint32_t layoutHash;
    
    bool operator==(const UIGeometryKey& other) const {
        return widgetId == other.widgetId && 
               styleHash == other.styleHash && 
               layoutHash == other.layoutHash;
    }
};

// Hash function for UIGeometryKey
struct APPLICATION_API UIGeometryKeyHash {
    std::size_t operator()(const UIGeometryKey& key) const {
        std::size_t h1 = std::hash<uint64_t>{}(key.widgetId);
        std::size_t h2 = std::hash<uint32_t>{}(key.styleHash);
        std::size_t h3 = std::hash<uint32_t>{}(key.layoutHash);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

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
