#include "Services/DiskThumbnailCache.h"
#include <fstream>

#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace we::editor::contentbrowser {

void DiskThumbnailCache::SetCacheDirectory(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_CacheDir = path;
    std::error_code ec;
    std::filesystem::create_directories(m_CacheDir, ec);
}

std::optional<BitmapRGBA> DiskThumbnailCache::TryLoad(const std::string& cacheKey, uint64_t sourceVersion) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_CacheDir.empty()) return std::nullopt;

    const auto metaPath = m_CacheDir / (cacheKey + ".meta");
    const auto imagePath = m_CacheDir / (cacheKey + ".png");
    if (!std::filesystem::exists(metaPath) || !std::filesystem::exists(imagePath)) return std::nullopt;

    std::ifstream meta(metaPath);
    uint64_t storedVersion = 0;
    meta >> storedVersion;
    if (storedVersion != sourceVersion) return std::nullopt;

    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(imagePath.string().c_str(), &w, &h, &channels, 4);
    if (!data) return std::nullopt;

    BitmapRGBA bmp;
    bmp.width = static_cast<uint32_t>(w);
    bmp.height = static_cast<uint32_t>(h);
    bmp.pixels.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return bmp;
}

void DiskThumbnailCache::Save(const std::string& cacheKey, uint64_t sourceVersion, const BitmapRGBA& bitmap) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_CacheDir.empty() || bitmap.pixels.empty()) return;

    const auto metaPath = m_CacheDir / (cacheKey + ".meta");
    const auto imagePath = m_CacheDir / (cacheKey + ".png");

    std::ofstream meta(metaPath, std::ios::trunc);
    meta << sourceVersion;

    stbi_write_png(imagePath.string().c_str(),
        static_cast<int>(bitmap.width),
        static_cast<int>(bitmap.height),
        4,
        bitmap.pixels.data(),
        static_cast<int>(bitmap.width) * 4);
}

} // namespace we::editor::contentbrowser
