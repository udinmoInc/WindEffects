#include "Rendering/Text/GlyphManager.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <array>

namespace we::UI::Text {

// ============================================================================
// GlyphManager Implementation
// ============================================================================

GlyphManager::GlyphManager() = default;

GlyphManager::~GlyphManager() {
    Shutdown();
}

bool GlyphManager::Initialize(const GlyphManagerConfig& config) {
    if (m_Initialized) {
        HE_WARN("GlyphManager: Already initialized");
        return true;
    }

    m_Config = config;
    m_GlyphCache.reserve(m_Config.maxCacheSize);
    m_Stats = {};
    m_CurrentFrame = 0;
    m_Initialized = true;

    HE_INFO("GlyphManager: Initialized with max cache size " + std::to_string(m_Config.maxCacheSize));
    return true;
}

void GlyphManager::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    ClearCache();
    m_MissingGlyphs.clear();
    m_Stats = {};
    m_Face = nullptr;
    m_Initialized = false;

    HE_INFO("GlyphManager: Shutdown");
}

void GlyphManager::SetFontFace(FT_FaceRec_* face) {
    m_Face = face;
}

bool GlyphManager::GetGlyph(uint32_t codepoint, GlyphMetrics& outMetrics) {
    if (!m_Initialized) {
        return false;
    }

    auto it = m_GlyphCache.find(codepoint);
    if (it != m_GlyphCache.end()) {
        // Cache hit
        outMetrics = it->second.metrics;
        it->second.lastAccessFrame = m_CurrentFrame;
        it->second.accessCount++;
        
        if (m_Config.trackStatistics) {
            m_Stats.cacheHits++;
            m_Stats.hitRate = static_cast<float>(m_Stats.cacheHits) / 
                             static_cast<float>(m_Stats.cacheHits + m_Stats.cacheMisses);
        }
        return true;
    }

    // Cache miss
    if (m_Config.trackStatistics) {
        m_Stats.cacheMisses++;
        m_Stats.hitRate = static_cast<float>(m_Stats.cacheHits) / 
                         static_cast<float>(m_Stats.cacheHits + m_Stats.cacheMisses);
    }

    // Try to load the glyph
    if (m_Config.autoLoadMissing) {
        if (LoadGlyph(codepoint)) {
            auto newIt = m_GlyphCache.find(codepoint);
            if (newIt != m_GlyphCache.end()) {
                outMetrics = newIt->second.metrics;
                return true;
            }
        }
    }

    return false;
}

bool GlyphManager::HasGlyph(uint32_t codepoint) const {
    return m_GlyphCache.find(codepoint) != m_GlyphCache.end();
}

bool GlyphManager::LoadGlyph(uint32_t codepoint) {
    if (!m_Initialized || !m_Face) {
        return false;
    }

    // Check if already cached
    if (HasGlyph(codepoint)) {
        return true;
    }

    // Evict LRU if cache is full
    if (m_Config.enableLRUEviction && IsCacheFull()) {
        EvictLRU();
    }

    // Check cache size again after eviction
    if (IsCacheFull()) {
        HE_WARN("GlyphManager: Cache full and eviction failed for codepoint U+" + 
                std::to_string(codepoint));
        return false;
    }

    GlyphMetrics metrics;
    if (!LoadGlyphFromFace(codepoint, metrics)) {
        m_MissingGlyphs.insert(codepoint);
        if (m_Config.trackStatistics) {
            m_Stats.missingGlyphs++;
        }
        return false;
    }

    // Add to cache
    GlyphCacheEntry entry;
    entry.metrics = metrics;
    entry.lastAccessFrame = m_CurrentFrame;
    entry.accessCount = 1;
    entry.isDirty = true;

    m_GlyphCache[codepoint] = entry;

    if (m_Config.trackStatistics) {
        m_Stats.totalGlyphs = m_GlyphCache.size();
        m_Stats.memoryUsageBytes = CalculateMemoryUsage();
    }

    return true;
}

size_t GlyphManager::LoadGlyphs(const std::vector<uint32_t>& codepoints) {
    size_t loaded = 0;
    for (uint32_t codepoint : codepoints) {
        if (LoadGlyph(codepoint)) {
            loaded++;
        }
    }
    return loaded;
}

size_t GlyphManager::PreloadCommonGlyphs() {
    std::vector<uint32_t> commonCodepoints = GetCommonCodepoints();
    return LoadGlyphs(commonCodepoints);
}

bool GlyphManager::LoadGlyphFromFace(uint32_t codepoint, GlyphMetrics& outMetrics) {
    if (!m_Face) {
        return false;
    }

    FT_UInt glyphIndex = FT_Get_Char_Index(m_Face, codepoint);
    if (glyphIndex == 0) {
        return false;
    }

    GlyphManagerUtils::ConvertMetrics(m_Face, glyphIndex, outMetrics);
    outMetrics.glyphIndex = glyphIndex;
    outMetrics.isWhitespace = GlyphManagerUtils::IsWhitespace(codepoint);
    outMetrics.hasBitmap = (m_Face->glyph->bitmap.width > 0 && m_Face->glyph->bitmap.rows > 0);

    return true;
}

bool GlyphManager::IsCacheFull() const {
    return m_GlyphCache.size() >= m_Config.maxCacheSize;
}

bool GlyphManager::RemoveGlyph(uint32_t codepoint) {
    auto it = m_GlyphCache.find(codepoint);
    if (it != m_GlyphCache.end()) {
        m_GlyphCache.erase(it);
        
        if (m_Config.trackStatistics) {
            m_Stats.totalGlyphs = m_GlyphCache.size();
            m_Stats.memoryUsageBytes = CalculateMemoryUsage();
        }
        return true;
    }
    return false;
}

void GlyphManager::ClearCache() {
    m_GlyphCache.clear();
    
    if (m_Config.trackStatistics) {
        m_Stats.totalGlyphs = 0;
        m_Stats.memoryUsageBytes = 0;
    }
}

std::vector<uint32_t> GlyphManager::GetCachedCodepoints() const {
    std::vector<uint32_t> codepoints;
    codepoints.reserve(m_GlyphCache.size());
    
    for (const auto& pair : m_GlyphCache) {
        codepoints.push_back(pair.first);
    }
    
    return codepoints;
}

size_t GlyphManager::GetGlyphCount() const {
    return m_GlyphCache.size();
}

GlyphManagerStats GlyphManager::GetStatistics() const {
    GlyphManagerStats stats = m_Stats;
    stats.totalGlyphs = m_GlyphCache.size();
    stats.memoryUsageBytes = CalculateMemoryUsage();
    
    if (stats.cacheHits + stats.cacheMisses > 0) {
        stats.hitRate = static_cast<float>(stats.cacheHits) / 
                       static_cast<float>(stats.cacheHits + stats.cacheMisses);
    }
    
    return stats;
}

void GlyphManager::ResetStatistics() {
    m_Stats.cacheHits = 0;
    m_Stats.cacheMisses = 0;
    m_Stats.missingGlyphs = 0;
    m_Stats.hitRate = 0.0f;
}

void GlyphManager::UpdateAtlasUV(uint32_t codepoint, float u0, float v0, float u1, float v1,
                                 int atlasX, int atlasY, int atlasWidth, int atlasHeight) {
    auto it = m_GlyphCache.find(codepoint);
    if (it != m_GlyphCache.end()) {
        it->second.metrics.u0 = u0;
        it->second.metrics.v0 = v0;
        it->second.metrics.u1 = u1;
        it->second.metrics.v1 = v1;
        it->second.metrics.atlasX = atlasX;
        it->second.metrics.atlasY = atlasY;
        it->second.metrics.atlasWidth = atlasWidth;
        it->second.metrics.atlasHeight = atlasHeight;
        it->second.isDirty = false;
    }
}

void GlyphManager::MarkDirty(uint32_t codepoint) {
    auto it = m_GlyphCache.find(codepoint);
    if (it != m_GlyphCache.end()) {
        it->second.isDirty = true;
    }
}

std::vector<uint32_t> GlyphManager::GetDirtyGlyphs() const {
    std::vector<uint32_t> dirtyGlyphs;
    
    for (const auto& pair : m_GlyphCache) {
        if (pair.second.isDirty) {
            dirtyGlyphs.push_back(pair.first);
        }
    }
    
    return dirtyGlyphs;
}

void GlyphManager::ClearDirtyFlags() {
    for (auto& pair : m_GlyphCache) {
        pair.second.isDirty = false;
    }
}

void GlyphManager::SetCurrentFrame(uint32_t frame) {
    m_CurrentFrame = frame;
}

size_t GlyphManager::EvictLRU() {
    if (!m_Config.enableLRUEviction) {
        return 0;
    }

    size_t evicted = 0;
    const size_t targetSize = m_Config.maxCacheSize * 3 / 4; // Evict to 75% capacity

    // Find LRU glyphs
    std::vector<std::pair<uint32_t, uint32_t>> lruList; // (codepoint, lastAccessFrame)
    lruList.reserve(m_GlyphCache.size());

    for (const auto& pair : m_GlyphCache) {
        lruList.emplace_back(pair.first, pair.second.lastAccessFrame);
    }

    // Sort by last access frame (oldest first)
    std::sort(lruList.begin(), lruList.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });

    // Evict oldest glyphs until we reach target size
    for (const auto& [codepoint, lastFrame] : lruList) {
        if (m_GlyphCache.size() <= targetSize) {
            break;
        }
        
        m_GlyphCache.erase(codepoint);
        evicted++;
    }

    if (evicted > 0) {
        HE_INFO("GlyphManager: Evicted " + std::to_string(evicted) + " LRU glyphs");
        
        if (m_Config.trackStatistics) {
            m_Stats.totalGlyphs = m_GlyphCache.size();
            m_Stats.memoryUsageBytes = CalculateMemoryUsage();
        }
    }

    return evicted;
}

std::vector<uint32_t> GlyphManager::GetMissingGlyphs() const {
    std::vector<uint32_t> missing;
    missing.reserve(m_MissingGlyphs.size());
    
    for (uint32_t codepoint : m_MissingGlyphs) {
        missing.push_back(codepoint);
    }
    
    return missing;
}

void GlyphManager::ClearMissingGlyphs() {
    m_MissingGlyphs.clear();
    if (m_Config.trackStatistics) {
        m_Stats.missingGlyphs = 0;
    }
}

size_t GlyphManager::CalculateMemoryUsage() const {
    size_t total = 0;
    
    for (const auto& pair : m_GlyphCache) {
        total += GlyphManagerUtils::EstimateEntryMemory(pair.second);
    }
    
    // Add hash map overhead
    total += m_GlyphCache.size() * sizeof(std::pair<uint32_t, GlyphCacheEntry>);
    total += m_GlyphCache.bucket_count() * sizeof(void*);
    
    return total;
}

std::vector<uint32_t> GlyphManager::GetCommonCodepoints() {
    std::vector<uint32_t> codepoints;
    
    // ASCII printable characters (32-126)
    for (uint32_t cp = 32; cp <= 126; ++cp) {
        codepoints.push_back(cp);
    }
    
    // Latin-1 supplement (160-255)
    for (uint32_t cp = 160; cp <= 255; ++cp) {
        codepoints.push_back(cp);
    }
    
    // Common punctuation and symbols
    const std::vector<uint32_t> commonSymbols = {
        0x2013, 0x2014, // en dash, em dash
        0x2018, 0x2019, // left single quote, right single quote
        0x201C, 0x201D, // left double quote, right double quote
        0x2022,         // bullet
        0x2026,         // ellipsis
        0x20AC,         // euro sign
        0x2122,         // trademark sign
        0x00A9,         // copyright sign
        0x00AE,         // registered sign
        0x00B0,         // degree sign
        0x00B1,         // plus-minus sign
        0x00D7,         // multiplication sign
        0x00F7,         // division sign
        0x2190, 0x2191, 0x2192, 0x2193, // arrows
        0x2212,         // minus sign
        0x2264, 0x2265, // less-than-or-equal, greater-than-or-equal
        0x00A0,         // non-breaking space
        0x00AD,         // soft hyphen
        0x200B,         // zero-width space
        0x200C, 0x200D, // zero-width non-joiner, zero-width joiner
        0xFEFF,         // zero-width no-break space (BOM)
    };
    
    codepoints.insert(codepoints.end(), commonSymbols.begin(), commonSymbols.end());
    
    return codepoints;
}

// ============================================================================
// GlyphManagerUtils Implementation
// ============================================================================

namespace GlyphManagerUtils {

bool IsWhitespace(uint32_t codepoint) {
    switch (codepoint) {
        case 0x0009: // TAB
        case 0x000A: // LINE FEED
        case 0x000B: // VERTICAL TAB
        case 0x000C: // FORM FEED
        case 0x000D: // CARRIAGE RETURN
        case 0x0020: // SPACE
        case 0x0085: // NEL
        case 0x00A0: // NO-BREAK SPACE
        case 0x1680: // OGHAM SPACE MARK
        case 0x2000: // EN QUAD
        case 0x2001: // EM QUAD
        case 0x2002: // EN SPACE
        case 0x2003: // EM SPACE
        case 0x2004: // THREE-PER-EM SPACE
        case 0x2005: // FOUR-PER-EM SPACE
        case 0x2006: // SIX-PER-EM SPACE
        case 0x2007: // FIGURE SPACE
        case 0x2008: // PUNCTUATION SPACE
        case 0x2009: // THIN SPACE
        case 0x200A: // HAIR SPACE
        case 0x2028: // LINE SEPARATOR
        case 0x2029: // PARAGRAPH SEPARATOR
        case 0x202F: // NARROW NO-BREAK SPACE
        case 0x205F: // MEDIUM MATHEMATICAL SPACE
        case 0x3000: // IDEOGRAPHIC SPACE
        case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            return true;
        default:
            return false;
    }
}

bool IsControl(uint32_t codepoint) {
    // C0 controls (0-31, 127)
    if (codepoint <= 0x1F || codepoint == 0x7F) {
        return true;
    }
    // C1 controls (128-159)
    if (codepoint >= 0x80 && codepoint <= 0x9F) {
        return true;
    }
    return false;
}

float GetAdvance(FT_FaceRec_* face, uint32_t codepoint) {
    if (!face) {
        return 0.0f;
    }

    FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
    if (glyphIndex == 0) {
        return 0.0f;
    }

    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
        return 0.0f;
    }

    return static_cast<float>(face->glyph->advance.x) / 64.0f;
}

float GetKerning(FT_FaceRec_* face, uint32_t leftCodepoint, uint32_t rightCodepoint) {
    if (!face) {
        return 0.0f;
    }

    FT_UInt leftIndex = FT_Get_Char_Index(face, leftCodepoint);
    FT_UInt rightIndex = FT_Get_Char_Index(face, rightCodepoint);

    if (leftIndex == 0 || rightIndex == 0) {
        return 0.0f;
    }

    if (!FT_HAS_KERNING(face)) {
        return 0.0f;
    }

    FT_Vector kerning;
    if (FT_Get_Kerning(face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning) != 0) {
        return 0.0f;
    }

    return static_cast<float>(kerning.x) / 64.0f;
}

void ConvertMetrics(FT_FaceRec_* face, uint32_t glyphIndex, GlyphMetrics& outMetrics) {
    if (!face) {
        outMetrics = {};
        return;
    }

    // Load the glyph
    FT_Error error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
    if (error != 0) {
        outMetrics = {};
        return;
    }

    const float scale = 1.0f / 64.0f; // Convert from 26.6 fixed-point to float

    // Get glyph metrics
    outMetrics.width = static_cast<float>(face->glyph->metrics.width) * scale;
    outMetrics.height = static_cast<float>(face->glyph->metrics.height) * scale;
    outMetrics.bearingX = static_cast<float>(face->glyph->metrics.horiBearingX) * scale;
    outMetrics.bearingY = static_cast<float>(face->glyph->metrics.horiBearingY) * scale;
    outMetrics.advanceX = static_cast<float>(face->glyph->metrics.horiAdvance) * scale;
    outMetrics.advanceY = static_cast<float>(face->glyph->metrics.vertAdvance) * scale;

    // Bitmap metrics (if available)
    if (face->glyph->bitmap.width > 0 && face->glyph->bitmap.rows > 0) {
        outMetrics.atlasWidth = face->glyph->bitmap.width;
        outMetrics.atlasHeight = face->glyph->bitmap.rows;
        outMetrics.hasBitmap = true;
    }
}

std::array<float, 4> CalculateBoundingBox(const GlyphMetrics& metrics) {
    return {
        metrics.bearingX,
        metrics.bearingY - metrics.height,
        metrics.bearingX + metrics.width,
        metrics.bearingY
    };
}

size_t EstimateEntryMemory(const GlyphCacheEntry& entry) {
    // Estimate memory usage for a cache entry
    size_t total = sizeof(GlyphCacheEntry);
    
    // Add estimate for hash map overhead
    total += sizeof(uint32_t); // key
    total += sizeof(void*);   // pointer overhead
    
    return total;
}

} // namespace GlyphManagerUtils

} // namespace we::UI::Text
