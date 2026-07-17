#include "WindEffects/Runtime/UI/Rendering/Icons/AtlasCache.h"

#include "Icons/Core/IconHash.h"
#include "WindEffects/Runtime/UI/Rendering/IconMetrics.h"

namespace WindEffects::Editor::UI {

void AtlasCache::LoadMeta(const std::vector<IconMetaEntryInput>& entries)
{
    std::scoped_lock lock(m_Mutex);
    m_Entries.clear();
    m_Entries.reserve(entries.size());
    for (const auto& entry : entries) {
        CachedIconEntry cached;
        cached.nameHash = entry.nameHash != 0 ? entry.nameHash : we::runtime::icons::Fnv1a64(entry.name);
        cached.tierPx = entry.tierPx;
        cached.uv = entry.uv;
        cached.flags = entry.flags;
        cached.lookupKey = we::runtime::icons::MakeIconLookupKey(cached.nameHash, cached.tierPx);
        m_Entries.push_back(cached);
    }
    RebuildLookup();
}

void AtlasCache::Clear()
{
    std::scoped_lock lock(m_Mutex);
    m_Entries.clear();
    m_Lookup.clear();
}

const CachedIconEntry* AtlasCache::Find(const std::string_view iconName, const uint32_t tierPx) const
{
    return FindByHash(we::runtime::icons::Fnv1a64(iconName), tierPx);
}

const CachedIconEntry* AtlasCache::FindByHash(const uint64_t nameHash, const uint32_t tierPx) const
{
    std::scoped_lock lock(m_Mutex);
    const uint64_t key = we::runtime::icons::MakeIconLookupKey(nameHash, tierPx);
    const auto it = m_Lookup.find(key);
    if (it == m_Lookup.end()) {
        return nullptr;
    }
    return &m_Entries[it->second];
}

const CachedIconEntry* AtlasCache::FindWithTierFallback(const std::string_view iconName, const uint32_t tierPx) const
{
    const uint64_t nameHash = we::runtime::icons::Fnv1a64(iconName);

    uint32_t startIndex = 0;
    for (uint32_t i = 0; i < IconMetrics::kAtlasTierCount; ++i) {
        if (IconMetrics::kAtlasTiers[i] >= tierPx) {
            startIndex = i;
            break;
        }
        startIndex = i;
    }

    // Prefer exact-or-higher tiers first, then fall back to smaller atlases.
    for (uint32_t i = startIndex; i < IconMetrics::kAtlasTierCount; ++i) {
        if (const CachedIconEntry* entry = FindByHash(nameHash, IconMetrics::kAtlasTiers[i])) {
            return entry;
        }
    }
    for (int i = static_cast<int>(startIndex) - 1; i >= 0; --i) {
        if (const CachedIconEntry* entry = FindByHash(nameHash, IconMetrics::kAtlasTiers[static_cast<uint32_t>(i)])) {
            return entry;
        }
    }

    return nullptr;
}

void AtlasCache::RebuildLookup()
{
    m_Lookup.clear();
    m_Lookup.reserve(m_Entries.size());
    for (size_t i = 0; i < m_Entries.size(); ++i) {
        m_Lookup.emplace(m_Entries[i].lookupKey, i);
    }
}

} // namespace WindEffects::Editor::UI
