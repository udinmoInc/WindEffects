#pragma once

#include "WindEffects/Editor/UI/Export.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace WindEffects::Editor::UI {

struct IconUvRect {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
};

enum class IconEntryFlags : uint8_t {
    None = 0,
    FullColor = 1 << 0,
};

inline bool HasIconFlag(IconEntryFlags flags, IconEntryFlags test)
{
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(test)) != 0;
}

struct CachedIconEntry {
    uint64_t lookupKey = 0;
    uint64_t nameHash = 0;
    uint32_t tierPx = 0;
    IconUvRect uv{};
    IconEntryFlags flags = IconEntryFlags::None;
};

struct IconMetaEntryInput {
    uint64_t nameHash = 0;
    std::string name;
    uint32_t tierPx = 0;
    IconUvRect uv{};
    IconEntryFlags flags = IconEntryFlags::None;
};

class UIFRAMEWORK_API AtlasCache {
public:
    AtlasCache() = default;

    void LoadMeta(const std::vector<IconMetaEntryInput>& entries);
    void Clear();

    [[nodiscard]] const CachedIconEntry* Find(std::string_view iconName, uint32_t tierPx) const;
    [[nodiscard]] const CachedIconEntry* FindByHash(uint64_t nameHash, uint32_t tierPx) const;
    [[nodiscard]] size_t EntryCount() const { return m_Entries.size(); }

private:
    void RebuildLookup();

    std::vector<CachedIconEntry> m_Entries;
    std::unordered_map<uint64_t, size_t> m_Lookup;
    mutable std::mutex m_Mutex;
};

} // namespace WindEffects::Editor::UI
