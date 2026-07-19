#pragma once

#include "ContentBrowser/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::contentbrowser {

struct CONTENTBROWSER_API ContentAssetId {
    std::string value;

    [[nodiscard]] bool IsValid() const noexcept { return !value.empty(); }
    [[nodiscard]] bool operator==(const ContentAssetId& o) const noexcept { return value == o.value; }
    [[nodiscard]] bool operator!=(const ContentAssetId& o) const noexcept { return !(*this == o); }
};

enum class ContentAssetKind : std::uint8_t {
    Unknown = 0,
    Folder,
    Texture,
    Material,
    MaterialInstance,
    StaticMesh,
    SkeletalMesh,
    Animation,
    Blueprint,
    Scene,
    Prefab,
    Terrain,
    Audio,
    Font,
    Script,
    Video,
    Redirector,
    Custom,
};

enum class ContentBrowserLayoutHint : std::uint8_t {
    Grid = 0,
    List,
    Columns,
};

enum class ContentSortMode : std::uint8_t {
    NameAsc = 0,
    NameDesc,
    TypeAsc,
    ModifiedDesc,
    SizeDesc,
};

enum class ContentSelectionOp : std::uint8_t {
    Replace = 0,
    Add,
    Toggle,
    Remove,
};

enum class ContentEventKind : std::uint8_t {
    SelectionChanged = 0,
    FolderChanged,
    CatalogRefreshed,
    AssetRenamed,
    AssetMoved,
    AssetDeleted,
    AssetImported,
    FilterChanged,
    ThumbnailReady,
};

struct CONTENTBROWSER_API ContentFilterState {
    bool showFolders = true;
    bool showTextures = true;
    bool showMeshes = true;
    bool showMaterials = true;
    bool showBlueprints = true;
    bool showAudio = true;
    bool showOther = true;
    bool favoritesOnly = false;
    bool recentOnly = false;
    ContentSortMode sortMode = ContentSortMode::NameAsc;
    std::string searchQuery;
    std::vector<std::string> tags;
};

struct CONTENTBROWSER_API ContentFilterPreset {
    std::string name;
    ContentFilterState state;
};

struct CONTENTBROWSER_API ContentBrowserConfig {
    bool multiSelectEnabled = true;
    bool virtualizeAssetView = true;
    bool thumbnailStreaming = true;
    bool liveFilesystemWatch = true;
    std::uint32_t recentLimit = 64;
    std::uint32_t historyLimit = 128;
};

} // namespace we::editor::contentbrowser
