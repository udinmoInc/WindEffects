// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserModel
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "RHI/Types.h"
#include <KindUI/EditorUI.h>
namespace we::editor::contentbrowser {

struct ContentItem {
    std::string id;
    std::string name;
    std::string type;
    std::string path;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
    we::rhi::RHIDescriptorSetHandle iconTexture = we::rhi::RHIDescriptorSetHandle::Invalid;
    bool isFolder = false;
    bool isFavorite = false;
    bool isDirty = false;
    bool thumbnailRequested = false;
    void* userData = nullptr;
};

enum class ContentViewMode {
    LargeIcons,
    MediumIcons,
    SmallIcons,
    Tiles,
    List,
    Details,
    Columns = Details,
    Grid = LargeIcons
};

class ContentBrowserModel {
public:
    std::vector<ContentItem> items;
    std::vector<std::string> selectedIds;
    std::string filterText;
    std::string currentFolder = "/Game";
    ContentViewMode viewMode = ContentViewMode::LargeIcons;

    size_t assetCount = 0;
    size_t folderCount = 0;
    size_t memoryUsageBytes = 0;

    std::function<void()> onModelChanged;

    void NotifyChanged() {
        if (onModelChanged) {
            onModelChanged();
        }
    }
};

} // namespace we::editor::contentbrowser