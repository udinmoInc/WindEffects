#include "KindUI/Rendering/Icons/AtlasCache.h"

#include "Icons/Core/IconHash.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <algorithm>

namespace we::runtime::kindui {

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

    if (const CachedIconEntry* exact = FindByHash(nameHash, tierPx)) {
        return exact;
    }

    // Walk only tiers that actually pack this icon — never global folder-only sizes.
    std::vector<uint32_t> tiers;
    {
        std::scoped_lock lock(m_Mutex);
        tiers.reserve(8);
        for (const CachedIconEntry& entry : m_Entries) {
            if (entry.nameHash == nameHash && entry.tierPx != 0) {
                tiers.push_back(entry.tierPx);
            }
        }
    }
    std::sort(tiers.begin(), tiers.end());
    tiers.erase(std::unique(tiers.begin(), tiers.end()), tiers.end());
    if (tiers.empty()) {
        return nullptr;
    }

    const uint32_t snapped = IconMetrics::SnapToTierList(tierPx, tiers);
    if (const CachedIconEntry* entry = FindByHash(nameHash, snapped)) {
        return entry;
    }

    auto startIt = std::lower_bound(tiers.begin(), tiers.end(), tierPx);
    const std::size_t startIndex = static_cast<std::size_t>(
        startIt == tiers.end() ? tiers.size() - 1
                               : static_cast<std::size_t>(startIt - tiers.begin()));

    for (std::size_t i = startIndex; i < tiers.size(); ++i) {
        if (const CachedIconEntry* entry = FindByHash(nameHash, tiers[i])) {
            return entry;
        }
    }
    for (std::size_t i = startIndex; i > 0; --i) {
        if (const CachedIconEntry* entry = FindByHash(nameHash, tiers[i - 1])) {
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

} // namespace we::runtime::kindui
