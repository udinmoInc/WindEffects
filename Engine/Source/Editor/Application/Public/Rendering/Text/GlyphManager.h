#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <functional>

struct FT_FaceRec_;

namespace we::UI::Text {

/**
 * @brief Glyph metrics for rendering
 */
struct GlyphMetrics {
    // Bounding box (in font units)
    float width = 0.0f;
    float height = 0.0f;
    float bearingX = 0.0f;  // Horizontal bearing from origin
    float bearingY = 0.0f;  // Vertical bearing from origin
    
    // Advance (in font units)
    float advanceX = 0.0f;
    float advanceY = 0.0f;
    
    // Atlas UV coordinates (normalized 0-1)
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    
    // Atlas position (in pixels)
    int atlasX = 0;
    int atlasY = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;
    
    // Glyph properties
    bool isWhitespace = false;
    bool hasBitmap = false;
    bool isColorGlyph = false;
    
    // FreeType glyph index
    uint32_t glyphIndex = 0;
};

/**
 * @brief Glyph cache entry
 */
struct GlyphCacheEntry {
    GlyphMetrics metrics;
    uint32_t lastAccessFrame = 0;
    uint32_t accessCount = 0;
    bool isDirty = false;
};

/**
 * @brief Glyph manager statistics
 */
struct GlyphManagerStats {
    size_t totalGlyphs = 0;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    size_t missingGlyphs = 0;
    float hitRate = 0.0f;
    size_t memoryUsageBytes = 0;
};

/**
 * @brief Glyph manager configuration
 */
struct GlyphManagerConfig {
    size_t maxCacheSize = 4096;           // Maximum glyphs in cache
    bool enableLRUEviction = true;       // Enable LRU eviction when cache is full
    bool trackStatistics = true;          // Track cache statistics
    bool autoLoadMissing = true;          // Automatically load missing glyphs
    uint32_t preloadCommonGlyphs = true; // Preload ASCII and common glyphs
};

/**
 * @brief Glyph manager interface
 * 
 * Manages glyph storage and retrieval using hash maps keyed by Unicode codepoints.
 * Provides efficient O(1) average lookup, caching, and statistics tracking.
 */
class APPLICATION_API IGlyphManager {
public:
    virtual ~IGlyphManager() = default;

    /**
     * @brief Initialize the glyph manager
     * @param config Configuration options
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const GlyphManagerConfig& config = {}) = 0;

    /**
     * @brief Shutdown the glyph manager
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Set the font face for glyph loading
     * @param face FreeType face
     */
    virtual void SetFontFace(FT_FaceRec_* face) = 0;

    /**
     * @brief Get glyph metrics for a codepoint
     * @param codepoint Unicode codepoint
     * @param outMetrics Output glyph metrics
     * @return true if found, false otherwise
     */
    virtual bool GetGlyph(uint32_t codepoint, GlyphMetrics& outMetrics) = 0;

    /**
     * @brief Check if a glyph exists in the cache
     * @param codepoint Unicode codepoint
     * @return true if exists, false otherwise
     */
    virtual bool HasGlyph(uint32_t codepoint) const = 0;

    /**
     * @brief Load a glyph into the cache
     * @param codepoint Unicode codepoint
     * @return true if successful, false otherwise
     */
    virtual bool LoadGlyph(uint32_t codepoint) = 0;

    /**
     * @brief Load multiple glyphs into the cache
     * @param codepoints Vector of Unicode codepoints
     * @return Number of successfully loaded glyphs
     */
    virtual size_t LoadGlyphs(const std::vector<uint32_t>& codepoints) = 0;

    /**
     * @brief Preload common glyphs (ASCII, Latin-1, etc.)
     * @return Number of glyphs loaded
     */
    virtual size_t PreloadCommonGlyphs() = 0;

    /**
     * @brief Remove a glyph from the cache
     * @param codepoint Unicode codepoint
     * @return true if removed, false if not found
     */
    virtual bool RemoveGlyph(uint32_t codepoint) = 0;

    /**
     * @brief Clear all glyphs from the cache
     */
    virtual void ClearCache() = 0;

    /**
     * @brief Get all cached codepoints
     * @return Vector of cached codepoints
     */
    virtual std::vector<uint32_t> GetCachedCodepoints() const = 0;

    /**
     * @brief Get the number of cached glyphs
     * @return Number of glyphs in cache
     */
    virtual size_t GetGlyphCount() const = 0;

    /**
     * @brief Get cache statistics
     * @return GlyphManagerStats structure
     */
    virtual GlyphManagerStats GetStatistics() const = 0;

    /**
     * @brief Reset cache statistics
     */
    virtual void ResetStatistics() = 0;

    /**
     * @brief Update atlas UV coordinates for a glyph
     * @param codepoint Unicode codepoint
     * @param u0 Left UV coordinate
     * @param v0 Top UV coordinate
     * @param u1 Right UV coordinate
     * @param v1 Bottom UV coordinate
     * @param atlasX Atlas X position in pixels
     * @param atlasY Atlas Y position in pixels
     * @param atlasWidth Atlas width in pixels
     * @param atlasHeight Atlas height in pixels
     */
    virtual void UpdateAtlasUV(uint32_t codepoint, float u0, float v0, float u1, float v1,
                               int atlasX, int atlasY, int atlasWidth, int atlasHeight) = 0;

    /**
     * @brief Mark a glyph as dirty (needs atlas update)
     * @param codepoint Unicode codepoint
     */
    virtual void MarkDirty(uint32_t codepoint) = 0;

    /**
     * @brief Get all dirty glyphs
     * @return Vector of dirty codepoints
     */
    virtual std::vector<uint32_t> GetDirtyGlyphs() const = 0;

    /**
     * @brief Clear dirty flags for all glyphs
     */
    virtual void ClearDirtyFlags() = 0;

    /**
     * @brief Set the current frame for LRU tracking
     * @param frame Current frame number
     */
    virtual void SetCurrentFrame(uint32_t frame) = 0;

    /**
     * @brief Evict least recently used glyphs if cache is full
     * @return Number of glyphs evicted
     */
    virtual size_t EvictLRU() = 0;

    /**
     * @brief Get missing glyphs (attempted but not found in font)
     * @return Vector of missing codepoints
     */
    virtual std::vector<uint32_t> GetMissingGlyphs() const = 0;

    /**
     * @brief Clear missing glyphs list
     */
    virtual void ClearMissingGlyphs() = 0;
};

/**
 * @brief Standard glyph manager implementation
 */
class APPLICATION_API GlyphManager : public IGlyphManager {
public:
    GlyphManager();
    ~GlyphManager() override;

    // Disable copying
    GlyphManager(const GlyphManager&) = delete;
    GlyphManager& operator=(const GlyphManager&) = delete;

    bool Initialize(const GlyphManagerConfig& config = {}) override;
    void Shutdown() override;

    void SetFontFace(FT_FaceRec_* face) override;

    bool GetGlyph(uint32_t codepoint, GlyphMetrics& outMetrics) override;
    bool HasGlyph(uint32_t codepoint) const override;
    bool LoadGlyph(uint32_t codepoint) override;
    size_t LoadGlyphs(const std::vector<uint32_t>& codepoints) override;
    size_t PreloadCommonGlyphs() override;

    bool RemoveGlyph(uint32_t codepoint) override;
    void ClearCache() override;

    std::vector<uint32_t> GetCachedCodepoints() const override;
    size_t GetGlyphCount() const override;

    GlyphManagerStats GetStatistics() const override;
    void ResetStatistics() override;

    void UpdateAtlasUV(uint32_t codepoint, float u0, float v0, float u1, float v1,
                       int atlasX, int atlasY, int atlasWidth, int atlasHeight) override;
    void MarkDirty(uint32_t codepoint) override;
    std::vector<uint32_t> GetDirtyGlyphs() const override;
    void ClearDirtyFlags() override;

    void SetCurrentFrame(uint32_t frame) override;
    size_t EvictLRU() override;

    std::vector<uint32_t> GetMissingGlyphs() const override;
    void ClearMissingGlyphs() override;

private:
    /**
     * @brief Load glyph from FreeType face
     * @param codepoint Unicode codepoint
     * @param outMetrics Output glyph metrics
     * @return true if successful, false otherwise
     */
    bool LoadGlyphFromFace(uint32_t codepoint, GlyphMetrics& outMetrics);

    /**
     * @brief Check if cache is full
     * @return true if full, false otherwise
     */
    bool IsCacheFull() const;

    /**
     * @brief Update statistics after a cache access
     * @param hit Whether the access was a hit
     */
    void UpdateStats(bool hit);

    /**
     * @brief Calculate memory usage of cache
     * @return Memory usage in bytes
     */
    size_t CalculateMemoryUsage() const;

    /**
     * @brief Get common codepoints to preload
     * @return Vector of common codepoints
     */
    static std::vector<uint32_t> GetCommonCodepoints();

    FT_FaceRec_* m_Face = nullptr;
    GlyphManagerConfig m_Config;
    
    // Hash map storage keyed by Unicode codepoint
    std::unordered_map<uint32_t, GlyphCacheEntry> m_GlyphCache;
    
    // Missing glyphs (attempted but not in font)
    std::unordered_set<uint32_t> m_MissingGlyphs;
    
    // Statistics
    mutable GlyphManagerStats m_Stats;
    uint32_t m_CurrentFrame = 0;
    bool m_Initialized = false;
};

/**
 * @brief Glyph manager utility functions
 */
namespace GlyphManagerUtils {
    /**
     * @brief Check if a codepoint is a whitespace character
     */
    bool IsWhitespace(uint32_t codepoint);

    /**
     * @brief Check if a codepoint is a control character
     */
    bool IsControl(uint32_t codepoint);

    /**
     * @brief Get the advance for a codepoint (without loading full glyph)
     * @param face FreeType face
     * @param codepoint Unicode codepoint
     * @return Advance in font units, or 0 if not found
     */
    float GetAdvance(FT_FaceRec_* face, uint32_t codepoint);

    /**
     * @brief Get the kerning between two codepoints
     * @param face FreeType face
     * @param leftCodepoint Left codepoint
     * @param rightCodepoint Right codepoint
     * @return Kerning adjustment in font units
     */
    float GetKerning(FT_FaceRec_* face, uint32_t leftCodepoint, uint32_t rightCodepoint);

    /**
     * @brief Convert glyph metrics from FreeType to internal format
     * @param face FreeType face
     * @param glyphIndex Glyph index
     * @param outMetrics Output metrics
     */
    void ConvertMetrics(FT_FaceRec_* face, uint32_t glyphIndex, GlyphMetrics& outMetrics);

    /**
     * @brief Calculate bounding box for a glyph
     * @param metrics Glyph metrics
     * @return Bounding box (x0, y0, x1, y1)
     */
    std::array<float, 4> CalculateBoundingBox(const GlyphMetrics& metrics);

    /**
     * @brief Estimate memory usage for a glyph cache entry
     * @param entry Glyph cache entry
     * @return Estimated memory usage in bytes
     */
    size_t EstimateEntryMemory(const GlyphCacheEntry& entry);
};

} // namespace we::UI::Text
