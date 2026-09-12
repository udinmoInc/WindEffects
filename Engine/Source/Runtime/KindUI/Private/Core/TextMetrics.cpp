// ==============================================================================
// WindEffects — KindUI — TextMetrics
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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

}

void TextMetrics::SetMeasureProvider(TextMetrics::MeasureFn provider) {
    std::scoped_lock lock(g_MeasureMutex);
    g_MeasureProvider = std::move(provider);
    g_MeasureCache.clear();
}

void TextMetrics::ClearCache() {
    std::scoped_lock lock(g_MeasureMutex);
    g_MeasureCache.clear();
}

FontMetricsSpec TextMetrics::GetFontMetrics(float fontSize) {
    const float scale = (fontSize > 0.0f) ? fontSize : 13.0f;
    FontMetricsSpec spec;
    spec.ascender = scale * 0.82f;
    spec.descender = scale * 0.22f;
    spec.capHeight = scale * 0.70f;
    spec.xHeight = scale * 0.50f;
    spec.lineHeight = scale * (32.0f / 24.0f);
    return spec;
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

std::string TextMetrics::TruncateText(
    const std::string_view text,
    const float maxWidth,
    const float fontSize,
    const bool bold,
    const TruncateMode mode)
{
    if (text.empty() || maxWidth <= 0.0f) {
        return "";
    }

    const float fullW = MeasureWidth(text, fontSize, bold);
    if (fullW <= maxWidth) {
        return std::string(text);
    }

    constexpr std::string_view kEllipsis = "...";
    const float ellipsisW = MeasureWidth(kEllipsis, fontSize, bold);
    if (ellipsisW >= maxWidth) {
        return std::string(kEllipsis);
    }

    const std::string str(text);

    if (mode == TruncateMode::Middle) {
        const auto dotPos = str.rfind('.');
        std::string suffix;
        std::string stem;

        if (dotPos != std::string::npos && dotPos > 0 && (str.size() - dotPos) <= 6) {
            suffix = str.substr(dotPos);
            stem = str.substr(0, dotPos);
        } else {
            stem = str;
        }

        const float suffixW = MeasureWidth(suffix, fontSize, bold);
        if (suffixW + ellipsisW < maxWidth) {
            for (size_t L = stem.size(); L > 0; --L) {
                std::string candidate = stem.substr(0, L) + "..." + suffix;
                if (MeasureWidth(candidate, fontSize, bold) <= maxWidth) {
                    return candidate;
                }
            }
        }
    }

    for (size_t L = str.size(); L > 0; --L) {
        std::string candidate = str.substr(0, L) + "...";
        if (MeasureWidth(candidate, fontSize, bold) <= maxWidth) {
            return candidate;
        }
    }

    return std::string(kEllipsis);
}

}

