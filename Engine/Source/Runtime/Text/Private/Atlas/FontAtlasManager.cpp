#include "Text/Atlas/FontAtlasManager.h"

#include <algorithm>
#include <cstring>

namespace we::runtime::text::atlas {
namespace {

constexpr uint32_t kGlyphPadding = 2;

class FontAtlasManager final : public IFontAtlasManager {
public:
    FontAtlasManager(uint32_t pageWidth, uint32_t pageHeight)
        : m_PageWidth(std::max(pageWidth, 256u))
        , m_PageHeight(std::max(pageHeight, 256u))
    {
        AddPage();
    }

    void SeedFromFontAsset(const FontHandle fontHandle, const assets::FontAsset& asset) override
    {
        std::lock_guard lock(m_Mutex);
        if (asset.atlasPages.empty()) {
            return;
        }

        // Fast path: adopt prebaked wefont atlas pages as dynamic pages (startup cache).
        // Glyph UVs from the asset remain valid against these pages.
        if (m_Pages.size() == 1
            && m_Pages.front().packX == kGlyphPadding
            && m_Pages.front().packY == kGlyphPadding
            && m_Cache.empty()) {
            m_Pages.clear();
        }

        const uint32_t basePage = static_cast<uint32_t>(m_Pages.size());
        for (const AtlasPage& src : asset.atlasPages) {
            AtlasPageRuntime runtime;
            runtime.page = src;
            runtime.version = 1;
            runtime.packX = runtime.page.width;
            runtime.packY = runtime.page.height;
            runtime.rowHeight = 0;
            runtime.dirty = true;
            m_Pages.push_back(std::move(runtime));
        }

        for (const GlyphMetrics& glyph : asset.glyphs) {
            GlyphAtlasKey key{fontHandle, glyph.codepoint, 0};
            AtlasGlyphEntry entry;
            entry.metrics = glyph;
            entry.metrics.atlasPage = static_cast<uint16_t>(basePage + glyph.atlasPage);
            entry.bakeSizePx = asset.metrics.bakeSizePx;
            entry.msdfPixelRange = asset.metrics.msdfPixelRange;
            entry.geometryScale = asset.metrics.geometryScale > 0.0f
                ? asset.metrics.geometryScale
                : 1.0f;
            entry.pageIndex = entry.metrics.atlasPage;
            entry.pageVersion = m_Pages[entry.pageIndex].version;
            m_Cache[key] = entry;
        }
        ++m_Generation;
    }

    std::optional<AtlasGlyphEntry> PackGlyph(
        const GlyphAtlasKey& key,
        const GlyphMetrics& sourceMetrics,
        std::span<const uint8_t> rgba,
        const uint32_t width,
        const uint32_t height,
        const float bakeSizePx,
        const float msdfPixelRange,
        const float geometryScale) override
    {
        std::lock_guard lock(m_Mutex);
        return PackGlyphUnlocked(
            key, sourceMetrics, rgba, width, height, bakeSizePx, msdfPixelRange, geometryScale);
    }

    std::optional<AtlasGlyphEntry> Find(const GlyphAtlasKey& key) const override
    {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Cache.find(key);
        if (it == m_Cache.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    void Touch(const GlyphAtlasKey& key) override
    {
        std::lock_guard lock(m_Mutex);
        if (auto it = m_Cache.find(key); it != m_Cache.end()) {
            it->second.lastUsedFrame = m_FrameIndex;
        }
    }

    void BeginFrame(const uint64_t frameIndex) override
    {
        std::lock_guard lock(m_Mutex);
        m_FrameIndex = frameIndex;
    }

    void EvictUnused(const size_t maxEntries) override
    {
        std::lock_guard lock(m_Mutex);
        if (m_Cache.size() <= maxEntries) {
            return;
        }

        std::vector<std::pair<GlyphAtlasKey, uint64_t>> ages;
        ages.reserve(m_Cache.size());
        for (const auto& [key, entry] : m_Cache) {
            ages.emplace_back(key, entry.lastUsedFrame);
        }
        std::sort(ages.begin(), ages.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        const size_t removeCount = m_Cache.size() - maxEntries;
        for (size_t i = 0; i < removeCount; ++i) {
            m_Cache.erase(ages[i].first);
        }
        for (auto& page : m_Pages) {
            ++page.version;
            page.dirty = true;
        }
        // Reset packer state after eviction — next packs grow cleanly.
        for (auto& page : m_Pages) {
            page.packX = kGlyphPadding;
            page.packY = kGlyphPadding;
            page.rowHeight = 0;
            std::fill(page.page.rgba.begin(), page.page.rgba.end(), static_cast<uint8_t>(0));
        }
        m_Cache.clear();
        ++m_Generation;
    }

    const AtlasPageRuntime* GetPage(const uint32_t pageIndex) const override
    {
        std::lock_guard lock(m_Mutex);
        if (pageIndex >= m_Pages.size()) {
            return nullptr;
        }
        return &m_Pages[pageIndex];
    }

    std::optional<AtlasPageRuntime> CopyPage(const uint32_t pageIndex) const override
    {
        std::lock_guard lock(m_Mutex);
        if (pageIndex >= m_Pages.size()) {
            return std::nullopt;
        }
        return m_Pages[pageIndex];
    }

    size_t PageCount() const override
    {
        std::lock_guard lock(m_Mutex);
        return m_Pages.size();
    }

    uint64_t Generation() const override
    {
        std::lock_guard lock(m_Mutex);
        return m_Generation;
    }

    std::vector<uint32_t> TakeDirtyPages() override
    {
        std::lock_guard lock(m_Mutex);
        std::vector<uint32_t> dirty;
        for (uint32_t i = 0; i < m_Pages.size(); ++i) {
            if (m_Pages[i].dirty) {
                dirty.push_back(i);
                m_Pages[i].dirty = false;
            }
        }
        return dirty;
    }

    void SetPageSize(const uint32_t width, const uint32_t height) override
    {
        std::lock_guard lock(m_Mutex);
        m_PageWidth = std::max(width, 256u);
        m_PageHeight = std::max(height, 256u);
    }

private:
    void AddPage()
    {
        AtlasPageRuntime runtime;
        runtime.page.width = m_PageWidth;
        runtime.page.height = m_PageHeight;
        runtime.page.format = AtlasFormat::Msdf;
        runtime.page.rgba.assign(static_cast<size_t>(m_PageWidth) * m_PageHeight * 4, 0);
        runtime.packX = kGlyphPadding;
        runtime.packY = kGlyphPadding;
        runtime.dirty = true;
        m_Pages.push_back(std::move(runtime));
    }

    std::optional<AtlasGlyphEntry> PackGlyphUnlocked(
        const GlyphAtlasKey& key,
        const GlyphMetrics& sourceMetrics,
        std::span<const uint8_t> rgba,
        const uint32_t width,
        const uint32_t height,
        const float bakeSizePx,
        const float msdfPixelRange,
        const float geometryScale)
    {
        if (auto existing = m_Cache.find(key); existing != m_Cache.end()) {
            existing->second.lastUsedFrame = m_FrameIndex;
            return existing->second;
        }

        if (width == 0 || height == 0 || rgba.size() < static_cast<size_t>(width) * height * 4) {
            return std::nullopt;
        }

        const uint32_t neededW = width + kGlyphPadding * 2;
        const uint32_t neededH = height + kGlyphPadding * 2;

        AtlasPageRuntime* page = nullptr;
        uint32_t pageIndex = 0;
        for (uint32_t i = 0; i < m_Pages.size(); ++i) {
            auto& candidate = m_Pages[i];
            if (candidate.packX + neededW > candidate.page.width) {
                candidate.packX = kGlyphPadding;
                candidate.packY += candidate.rowHeight + kGlyphPadding;
                candidate.rowHeight = 0;
            }
            if (candidate.packY + neededH <= candidate.page.height) {
                page = &candidate;
                pageIndex = i;
                break;
            }
        }
        if (!page) {
            AddPage();
            pageIndex = static_cast<uint32_t>(m_Pages.size() - 1);
            page = &m_Pages.back();
        }

        const uint32_t dstX = page->packX + kGlyphPadding;
        const uint32_t dstY = page->packY + kGlyphPadding;
        for (uint32_t y = 0; y < height; ++y) {
            uint8_t* dst = page->page.rgba.data()
                + (static_cast<size_t>(dstY + y) * page->page.width + dstX) * 4;
            const uint8_t* src = rgba.data() + static_cast<size_t>(y) * width * 4;
            std::memcpy(dst, src, static_cast<size_t>(width) * 4);
        }

        page->packX += neededW;
        page->rowHeight = std::max(page->rowHeight, neededH);
        page->dirty = true;

        GlyphMetrics metrics = sourceMetrics;
        metrics.atlasPage = static_cast<uint16_t>(pageIndex);
        metrics.atlasUv.u0 = static_cast<float>(dstX) / static_cast<float>(page->page.width);
        metrics.atlasUv.v0 = static_cast<float>(dstY) / static_cast<float>(page->page.height);
        metrics.atlasUv.u1 = static_cast<float>(dstX + width) / static_cast<float>(page->page.width);
        metrics.atlasUv.v1 = static_cast<float>(dstY + height) / static_cast<float>(page->page.height);
        metrics.hasDrawableQuad = true;

        AtlasGlyphEntry entry;
        entry.metrics = metrics;
        entry.bakeSizePx = bakeSizePx;
        entry.msdfPixelRange = msdfPixelRange;
        entry.geometryScale = geometryScale;
        entry.pageIndex = pageIndex;
        entry.pageVersion = page->version;
        entry.lastUsedFrame = m_FrameIndex;
        m_Cache[key] = entry;
        ++m_Generation;
        return entry;
    }

    mutable std::mutex m_Mutex;
    uint32_t m_PageWidth = 2048;
    uint32_t m_PageHeight = 2048;
    std::vector<AtlasPageRuntime> m_Pages;
    std::unordered_map<GlyphAtlasKey, AtlasGlyphEntry, GlyphAtlasKeyHash> m_Cache;
    uint64_t m_Generation = 1;
    uint64_t m_FrameIndex = 0;
};

} // namespace

std::unique_ptr<IFontAtlasManager> CreateFontAtlasManager(
    const uint32_t pageWidth,
    const uint32_t pageHeight)
{
    return std::make_unique<FontAtlasManager>(pageWidth, pageHeight);
}

} // namespace we::runtime::text::atlas
