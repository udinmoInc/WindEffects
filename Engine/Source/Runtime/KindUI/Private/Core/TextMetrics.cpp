#include "KindUI/Core/TextMetrics.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace we::runtime::kindui {

namespace {

struct CacheKey {
    std::string text;
    float fontSize = 0.0f;
    bool bold = false;

    bool operator==(const CacheKey& other) const {
        return bold == other.bold
            && fontSize == other.fontSize
            && text == other.text;
    }
};

struct CacheKeyHash {
    size_t operator()(const CacheKey& key) const noexcept {
        size_t h = std::hash<std::string>{}(key.text);
        h ^= std::hash<float>{}(key.fontSize) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<bool>{}(key.bold) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

std::mutex g_MeasureMutex;
TextMetrics::MeasureFn g_MeasureProvider;
std::unordered_map<CacheKey, float, CacheKeyHash> g_MeasureCache;
constexpr size_t kMaxCacheEntries = 4096;

void TrimCacheIfNeeded() {
    if (g_MeasureCache.size() <= kMaxCacheEntries) {
        return;
    }
    const size_t removeCount = g_MeasureCache.size() / 4;
    auto it = g_MeasureCache.begin();
    for (size_t i = 0; i < removeCount && it != g_MeasureCache.end(); ++i) {
        it = g_MeasureCache.erase(it);
    }
}

} // namespace

void TextMetrics::SetMeasureProvider(TextMetrics::MeasureFn provider) {
    std::scoped_lock lock(g_MeasureMutex);
    g_MeasureProvider = std::move(provider);
    g_MeasureCache.clear();
}

void TextMetrics::ClearCache() {
    std::scoped_lock lock(g_MeasureMutex);
    g_MeasureCache.clear();
}

float TextMetrics::MeasureWidth(const std::string_view text, const float fontSize, const bool bold) {
    if (text.empty()) {
        return 0.0f;
    }

    CacheKey key;
    key.text.assign(text.begin(), text.end());
    key.fontSize = fontSize;
    key.bold = bold;

    {
        std::scoped_lock lock(g_MeasureMutex);
        if (const auto found = g_MeasureCache.find(key); found != g_MeasureCache.end()) {
            return found->second;
        }
    }

    TextMetrics::MeasureFn provider;
    float width = 0.0f;
    {
        std::scoped_lock lock(g_MeasureMutex);
        provider = g_MeasureProvider;
    }
    if (provider) {
        width = provider(text, fontSize, bold);
    } else {
        width = static_cast<float>(text.size()) * fontSize * 0.5f;
    }

    {
        std::scoped_lock lock(g_MeasureMutex);
        TrimCacheIfNeeded();
        g_MeasureCache.emplace(std::move(key), width);
    }
    return width;
}

float TextMetrics::EstimateWidth(const std::string_view text, const float fontSize) {
    return MeasureWidth(text, fontSize, false);
}

float TextMetrics::CharWidth(const float fontSize) {
    return MeasureWidth("M", fontSize, false);
}

} // namespace we::runtime::kindui
