#pragma once

#include "Services/ThumbnailRenderer.hpp"
#include <string>
#include <unordered_map>

namespace we::editor::contentbrowser {

// Folders use a single shared flat icon. Opening a folder shows asset thumbnails in the grid.
class FolderPreviewGenerator {
public:
    BitmapRGBA Generate(const std::string& folderVirtualPath, uint32_t folderVersion);
    void Invalidate(const std::string& folderVirtualPath);
    void InvalidateAll();

private:
    std::unordered_map<std::string, BitmapRGBA> m_BitmapCache;
    std::unordered_map<std::string, uint32_t> m_VersionCache;
};

} // namespace we::editor::contentbrowser
