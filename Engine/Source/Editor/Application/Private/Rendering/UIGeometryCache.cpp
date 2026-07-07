#include "Rendering/UIGeometryCache.h"
#include "Core/Logger.h"
#include <algorithm>
#include <cstring>

namespace we::UI {

UIGeometryCache::UIGeometryCache()
    : m_MaxCacheSizeBytes(64 * 1024 * 1024) // 64MB default
    , m_CurrentCacheSizeBytes(0)
    , m_CacheHits(0)
    , m_CacheMisses(0)
{
}

UIGeometryCache::~UIGeometryCache() {
    Shutdown();
}

void UIGeometryCache::Initialize(uint32_t maxCacheSizeMB) {
    m_MaxCacheSizeBytes = static_cast<size_t>(maxCacheSizeMB) * 1024 * 1024;
    m_Cache.clear();
    m_CurrentCacheSizeBytes = 0;
    m_CacheHits = 0;
    m_CacheMisses = 0;
}

void UIGeometryCache::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cache.clear();
    m_CurrentCacheSizeBytes = 0;
}

bool UIGeometryCache::GetGeometry(const UIGeometryKey& key,
                                   std::vector<UIVertex2>& outVertices,
                                   std::vector<uint32_t>& outIndices) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Cache.find(key);
    if (it != m_Cache.end() && !it->second.isDirty) {
        outVertices = it->second.vertices;
        outIndices = it->second.indices;
        m_CacheHits++;
        return true;
    }
    
    m_CacheMisses++;
    return false;
}

void UIGeometryCache::CacheGeometry(const UIGeometryKey& key,
                                    const std::vector<UIVertex2>& vertices,
                                    const std::vector<uint32_t>& indices,
                                    uint32_t frameIndex) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // Calculate size
    size_t geometrySize = vertices.size() * sizeof(UIVertex2) + 
                         indices.size() * sizeof(uint32_t);
    
    // Check if we need to evict
    while (m_CurrentCacheSizeBytes + geometrySize > m_MaxCacheSizeBytes && !m_Cache.empty()) {
        EvictLRU();
    }
    
    // Cache the geometry
    UIGeometryEntry entry;
    entry.vertices = vertices;
    entry.indices = indices;
    entry.lastUsedFrame = frameIndex;
    entry.isDirty = false;
    
    m_Cache[key] = std::move(entry);
    m_CurrentCacheSizeBytes += geometrySize;
}

void UIGeometryCache::InvalidateGeometry(const UIGeometryKey& key) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_Cache.find(key);
    if (it != m_Cache.end()) {
        m_CurrentCacheSizeBytes -= (it->second.vertices.size() * sizeof(UIVertex2) + 
                                   it->second.indices.size() * sizeof(uint32_t));
        m_Cache.erase(it);
    }
}

void UIGeometryCache::InvalidateWidget(uint64_t widgetId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ) {
        if (it->first.widgetId == widgetId) {
            m_CurrentCacheSizeBytes -= (it->second.vertices.size() * sizeof(UIVertex2) + 
                                       it->second.indices.size() * sizeof(uint32_t));
            it = m_Cache.erase(it);
        } else {
            ++it;
        }
    }
}

void UIGeometryCache::InvalidateAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cache.clear();
    m_CurrentCacheSizeBytes = 0;
}

size_t UIGeometryCache::GetCacheSizeBytes() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CurrentCacheSizeBytes;
}

size_t UIGeometryCache::GetCacheEntryCount() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Cache.size();
}

void UIGeometryCache::ResetStats() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CacheHits = 0;
    m_CacheMisses = 0;
}

void UIGeometryCache::EvictOldEntries(uint32_t currentFrame, uint32_t maxAge) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ) {
        if (currentFrame - it->second.lastUsedFrame > maxAge) {
            m_CurrentCacheSizeBytes -= (it->second.vertices.size() * sizeof(UIVertex2) + 
                                       it->second.indices.size() * sizeof(uint32_t));
            it = m_Cache.erase(it);
        } else {
            ++it;
        }
    }
}

void UIGeometryCache::CompactCache() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // Remove dirty entries
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ) {
        if (it->second.isDirty) {
            m_CurrentCacheSizeBytes -= (it->second.vertices.size() * sizeof(UIVertex2) + 
                                       it->second.indices.size() * sizeof(uint32_t));
            it = m_Cache.erase(it);
        } else {
            ++it;
        }
    }
}

void UIGeometryCache::EvictLRU() {
    if (m_Cache.empty()) return;
    
    // Find least recently used entry
    auto lruIt = m_Cache.begin();
    uint32_t oldestFrame = lruIt->second.lastUsedFrame;
    
    for (auto it = m_Cache.begin(); it != m_Cache.end(); ++it) {
        if (it->second.lastUsedFrame < oldestFrame) {
            oldestFrame = it->second.lastUsedFrame;
            lruIt = it;
        }
    }
    
    m_CurrentCacheSizeBytes -= (lruIt->second.vertices.size() * sizeof(UIVertex2) + 
                               lruIt->second.indices.size() * sizeof(uint32_t));
    m_Cache.erase(lruIt);
}

} // namespace we::UI
