// ==============================================================================
// WindEffects — ContentBrowser — FolderPreviewGenerator
// Internal implementation for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Services/ThumbnailRenderer.h"
#include <string>
#include <unordered_map>

namespace we::editor::contentbrowser {


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
