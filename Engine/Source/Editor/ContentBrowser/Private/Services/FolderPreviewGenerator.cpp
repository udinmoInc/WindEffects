#include "Services/FolderPreviewGenerator.hpp"
#include "Services/ThumbnailRenderer.hpp"
#include "Services/ThumbnailRenderer.hpp"

namespace we::editor::contentbrowser {

BitmapRGBA FolderPreviewGenerator::Generate(const std::string& folderVirtualPath, uint32_t folderVersion) {
    (void)folderVirtualPath;
    auto it = m_VersionCache.find(folderVirtualPath);
    if (it != m_VersionCache.end() && it->second == folderVersion) {
        auto cached = m_BitmapCache.find(folderVirtualPath);
        if (cached != m_BitmapCache.end()) return cached->second;
    }

    // UE5: folders are quiet flat icons — content previews appear when the folder is opened.
    BitmapRGBA preview = ThumbnailRenderer::RenderContentBrowserFolderThumbnail();
    m_BitmapCache[folderVirtualPath] = preview;
    m_VersionCache[folderVirtualPath] = folderVersion;
    return preview;
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
