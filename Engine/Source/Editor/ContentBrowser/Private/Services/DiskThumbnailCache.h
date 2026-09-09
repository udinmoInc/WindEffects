// ==============================================================================
// WindEffects — ContentBrowser — DiskThumbnailCache
// Internal implementation for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Services/ThumbnailRenderer.h"
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace we::editor::contentbrowser {

class DiskThumbnailCache {
public:
    void SetCacheDirectory(const std::filesystem::path& path);
    std::optional<BitmapRGBA> TryLoad(const std::string& cacheKey, uint64_t sourceVersion) const;
    void Save(const std::string& cacheKey, uint64_t sourceVersion, const BitmapRGBA& bitmap) const;

private:
    std::filesystem::path m_CacheDir;
    mutable std::mutex m_Mutex;
};

} // namespace we::editor::contentbrowser
