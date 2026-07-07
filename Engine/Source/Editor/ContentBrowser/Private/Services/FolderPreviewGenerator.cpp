#include "Services/FolderPreviewGenerator.h"
#include "Services/ThumbnailRenderer.h"

namespace we::editor::contentbrowser {

BitmapRGBA FolderPreviewGenerator::Generate(const std::string& folderVirtualPath, uint32_t folderVersion) {
    (void)folderVirtualPath;
    auto it = m_VersionCache.find(folderVirtualPath);
    if (it != m_VersionCache.end() && it->second == folderVersion) {
        auto cached = m_BitmapCache.find(folderVirtualPath);
        if (cached != m_BitmapCache.end()) return cached->second;
    }

    BitmapRGBA bmp = ThumbnailRenderer::RenderContentBrowserFolder(ThumbnailRenderer::kThumbnailSize, 0.0f);
    m_BitmapCache[folderVirtualPath] = bmp;
    m_VersionCache[folderVirtualPath] = folderVersion;
    return bmp;
}

void FolderPreviewGenerator::Invalidate(const std::string& folderVirtualPath) {
    m_BitmapCache.erase(folderVirtualPath);
    m_VersionCache.erase(folderVirtualPath);
}

void FolderPreviewGenerator::InvalidateAll() {
    m_BitmapCache.clear();
    m_VersionCache.clear();
}

} // namespace we::editor::contentbrowser
