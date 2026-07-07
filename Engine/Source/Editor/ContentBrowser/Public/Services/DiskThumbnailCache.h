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
