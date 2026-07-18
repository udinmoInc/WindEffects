#pragma once

#include "Registry/AssetTypes.h"
#include <cstdint>
#include <string>

namespace we::editor::contentbrowser {

enum class ContentFilter : uint32_t {
    None = 0,
    Textures = 1 << 0,
    Materials = 1 << 1,
    Meshes = 1 << 2,
    Blueprints = 1 << 3,
    Scenes = 1 << 4,
    Scripts = 1 << 5,
    Animations = 1 << 6,
    Audio = 1 << 7,
    Video = 1 << 8,
    Fonts = 1 << 9,
    Favorites = 1 << 10,
    RecentlyModified = 1 << 11,
    Referenced = 1 << 12,
    Unused = 1 << 13,
    Folders = 1 << 14
};

inline ContentFilter operator|(ContentFilter a, ContentFilter b) {
    return static_cast<ContentFilter>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ContentFilter& operator|=(ContentFilter& a, ContentFilter b) {
    a = a | b;
    return a;
}

inline bool HasFilter(ContentFilter mask, ContentFilter flag) {
    return (static_cast<uint32_t>(mask) & static_cast<uint32_t>(flag)) != 0;
}

class FilterController {
public:
    void SetActiveFilters(ContentFilter filters) { m_ActiveFilters = filters; }
    ContentFilter GetActiveFilters() const { return m_ActiveFilters; }
    void ToggleFilter(ContentFilter filter);
    bool Matches(const AssetRecord& asset) const;

private:
    static bool MatchesTypeFilter(const AssetRecord& asset, ContentFilter filters);

    ContentFilter m_ActiveFilters = ContentFilter::None;
};

} // namespace we::editor::contentbrowser
