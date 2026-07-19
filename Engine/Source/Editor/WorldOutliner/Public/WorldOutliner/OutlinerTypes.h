#pragma once

#include "WorldOutliner/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::outliner {

/// Opaque outliner identity — Scene entity Id near-term; World ActorHandle later.
struct WORLDOUTLINER_API OutlinerNodeId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const OutlinerNodeId& o) const noexcept {
        return value == o.value;
    }
    [[nodiscard]] constexpr bool operator!=(const OutlinerNodeId& o) const noexcept {
        return !(*this == o);
    }
};

enum class OutlinerNodeKind : std::uint8_t {
    Unknown = 0,
    World,
    Level,
    StreamingLevel,
    PartitionCell,
    Folder,
    Actor,
    Component,
    PrefabInstance,
    Custom,
};

enum class OutlinerColumnId : std::uint8_t {
    Name = 0,
    Type,
    Layer,
    Tag,
    Visibility,
    Lock,
    Custom,
};

enum class OutlinerSelectionOp : std::uint8_t {
    Replace = 0,
    Add,
    Toggle,
    Remove,
};

enum class OutlinerSortMode : std::uint8_t {
    NameAsc = 0,
    NameDesc,
    TypeAsc,
    RecentlyModified,
};

enum class OutlinerEventKind : std::uint8_t {
    SelectionChanged = 0,
    HierarchyChanged,
    NodeRenamed,
    NodeReparented,
    NodeDeleted,
    FilterChanged,
    ModelRebuilt,
    UndoBoundary,
};

struct WORLDOUTLINER_API OutlinerFilterState {
    bool showFolders = true;
    bool showActors = true;
    bool showComponents = true;
    bool showHidden = false;
    bool showLocked = true;
    bool showEmptyFolders = true;
    bool favoritesOnly = false;
    OutlinerSortMode sortMode = OutlinerSortMode::NameAsc;
    std::string searchQuery;
};

struct WORLDOUTLINER_API OutlinerNodeFlags {
    bool visible = true;
    bool locked = false;
    bool expandable = true;
    bool expanded = false;
    bool favorite = false;
    bool pinned = false;
    bool lazyChildren = false;
};

struct WORLDOUTLINER_API OutlinerConfig {
    bool multiSelectEnabled = true;
    bool virtualizeRows = true;
    bool lazyLoadChildren = true;
    std::uint32_t selectionHistoryLimit = 64;
    std::uint32_t maxVisibleHint = 10'000;
};

struct WORLDOUTLINER_API OutlinerFilterPreset {
    std::string name;
    OutlinerFilterState state;
};

} // namespace we::editor::outliner
