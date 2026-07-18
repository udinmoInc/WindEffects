#include "Reflection/PropertyPath.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/NameId.h"
#include "Reflection/TypeKinds.h"
#include "HashUtils.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

namespace we::runtime::reflection {
namespace {

struct PathCacheKey {
    TypeId rootTypeId = kInvalidTypeId;
    std::uint64_t pathHash = 0;

    [[nodiscard]] bool operator==(const PathCacheKey& other) const noexcept {
        return rootTypeId == other.rootTypeId && pathHash == other.pathHash;
    }
};

struct PathCacheKeyHash {
    [[nodiscard]] std::size_t operator()(const PathCacheKey& key) const noexcept {
        return static_cast<std::size_t>(key.rootTypeId ^ (key.pathHash * 0x9e3779b97f4a7c15ull));
    }
};

struct PathCacheState {
    std::shared_mutex mutex;
    std::unordered_map<PathCacheKey, ResolvedPropertyPath, PathCacheKeyHash> entries;
    std::atomic<std::uint64_t> hits{0};
    std::atomic<std::uint64_t> misses{0};
};

PathCacheState& Cache() {
    static PathCacheState state;
    return state;
}

std::uint64_t HashSegments(const std::vector<NameId>& segments) {
    std::uint64_t hash = detail::Fnv1a64("we.reflection.path.v1");
    for (NameId id : segments) {
        hash ^= static_cast<std::uint64_t>(id);
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1ull : hash;
}

ResolvedPropertyPath ResolveSegments(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    const std::vector<PropertyPathSegment>& segments)
{
    ResolvedPropertyPath result;
    result.rootTypeId = rootTypeId;
    result.segments = segments;
    if (segments.empty() || rootTypeId == kInvalidTypeId) {
        return result;
    }

    TypeId currentType = rootTypeId;
    std::uint32_t absoluteOffset = 0;
    bool allDirect = true;

    for (const PropertyPathSegment& segment : segments) {
        const PropertyInfo* property = registry.FindProperty(currentType, segment.nameId);
        if (!property) {
            return result;
        }
        result.properties.push_back(property);
        if (!property->HasDirectStorage()) {
            allDirect = false;
        } else {
            absoluteOffset += property->offset;
        }

        currentType = property->typeId;
        if (currentType == kInvalidTypeId) {
            // Leaf primitive without nested type — allow as terminal.
            break;
        }
    }

    if (result.properties.size() != segments.size()) {
        return result;
    }

    result.leafTypeId = result.properties.back()->typeId;
    result.absoluteOffset = absoluteOffset;
    result.allDirectStorage = allDirect;
    result.valid = true;
    return result;
}

bool GetLeafRaw(const PropertyInfo& property, const void* leafInstance, void* outValue, std::size_t outSize) {
    if (!leafInstance || !outValue || outSize == 0) {
        return false;
    }
    if (property.getter) {
        return property.getter(leafInstance, outValue, outSize);
    }
    if (property.size == 0 || outSize < property.size) {
        return false;
    }
    const void* src = property.ConstPtr(leafInstance);
    if (!src) {
        return false;
    }
    std::memcpy(outValue, src, property.size);
    return true;
}

bool SetLeafRaw(const PropertyInfo& property, void* leafInstance, const void* value, std::size_t valueSize) {
    if (!leafInstance || !value || valueSize == 0) {
        return false;
    }
    if (HasFlag(property.flags, PropertyFlags::ReadOnly) && !property.setter) {
        return false;
    }
    if (property.setter) {
        return property.setter(leafInstance, value, valueSize);
    }
    if (property.size == 0 || valueSize < property.size) {
        return false;
    }
    void* dst = property.MutablePtr(leafInstance);
    if (!dst) {
        return false;
    }
    std::memcpy(dst, value, property.size);
    return true;
}

} // namespace

std::vector<PropertyPathSegment> ParsePropertyPath(std::string_view path) {
    std::vector<PropertyPathSegment> segments;
    if (path.empty()) {
        return segments;
    }
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t dot = path.find('.', start);
        const std::string_view part =
            path.substr(start, dot == std::string_view::npos ? path.size() - start : dot - start);
        if (part.empty()) {
            return {};
        }
        PropertyPathSegment segment;
        segment.name = std::string(part);
        segment.nameId = InternName(part);
        segments.push_back(std::move(segment));
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return segments;
}

ResolvedPropertyPath ResolvePropertyPath(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    std::string_view path)
{
    const auto segments = ParsePropertyPath(path);
    std::vector<NameId> ids;
    ids.reserve(segments.size());
    for (const PropertyPathSegment& segment : segments) {
        ids.push_back(segment.nameId);
    }
    return ResolvePropertyPath(registry, rootTypeId, ids);
}

ResolvedPropertyPath ResolvePropertyPath(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    const std::vector<NameId>& segmentNameIds)
{
    if (segmentNameIds.empty()) {
        return {};
    }

    PathCacheKey key;
    key.rootTypeId = rootTypeId;
    key.pathHash = HashSegments(segmentNameIds);

    auto& cache = Cache();
    {
        std::shared_lock lock(cache.mutex);
        const auto it = cache.entries.find(key);
        if (it != cache.entries.end()) {
            cache.hits.fetch_add(1, std::memory_order_relaxed);
            return it->second;
        }
    }

    cache.misses.fetch_add(1, std::memory_order_relaxed);

    std::vector<PropertyPathSegment> segments;
    segments.reserve(segmentNameIds.size());
    for (NameId id : segmentNameIds) {
        PropertyPathSegment segment;
        segment.nameId = id;
        segment.name = std::string(ResolveName(id));
        segments.push_back(std::move(segment));
    }

    ResolvedPropertyPath resolved = ResolveSegments(registry, rootTypeId, segments);
    if (resolved.valid) {
        std::unique_lock lock(cache.mutex);
        cache.entries.emplace(key, resolved);
    }
    return resolved;
}

void* ResolvePropertyPathInstance(void* rootInstance, const ResolvedPropertyPath& path) {
    if (!rootInstance || !path.valid || !path.allDirectStorage || path.properties.empty()) {
        return nullptr;
    }
    return static_cast<std::uint8_t*>(rootInstance) + path.absoluteOffset;
}

const void* ResolvePropertyPathInstance(const void* rootInstance, const ResolvedPropertyPath& path) {
    return ResolvePropertyPathInstance(const_cast<void*>(rootInstance), path);
}

bool GetPropertyPathRaw(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    const void* instance,
    std::string_view path,
    void* outValue,
    std::size_t outSize)
{
    if (!instance) {
        return false;
    }
    const ResolvedPropertyPath resolved = ResolvePropertyPath(registry, rootTypeId, path);
    if (!resolved.valid || !resolved.Leaf()) {
        return false;
    }

    const void* cursor = instance;
    for (std::size_t i = 0; i + 1 < resolved.properties.size(); ++i) {
        const PropertyInfo* property = resolved.properties[i];
        if (!property->HasDirectStorage()) {
            return false;
        }
        cursor = property->ConstPtr(cursor);
        if (!cursor) {
            return false;
        }
    }
    return GetLeafRaw(*resolved.Leaf(), cursor, outValue, outSize);
}

bool SetPropertyPathRaw(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    void* instance,
    std::string_view path,
    const void* value,
    std::size_t valueSize)
{
    if (!instance) {
        return false;
    }
    const ResolvedPropertyPath resolved = ResolvePropertyPath(registry, rootTypeId, path);
    if (!resolved.valid || !resolved.Leaf()) {
        return false;
    }

    void* cursor = instance;
    for (std::size_t i = 0; i + 1 < resolved.properties.size(); ++i) {
        const PropertyInfo* property = resolved.properties[i];
        if (!property->HasDirectStorage()) {
            return false;
        }
        cursor = property->MutablePtr(cursor);
        if (!cursor) {
            return false;
        }
    }
    return SetLeafRaw(*resolved.Leaf(), cursor, value, valueSize);
}

void ClearPropertyPathCache() {
    auto& cache = Cache();
    std::unique_lock lock(cache.mutex);
    cache.entries.clear();
}

PropertyPathCacheStats GetPropertyPathCacheStats() {
    auto& cache = Cache();
    PropertyPathCacheStats stats;
    stats.hits = cache.hits.load(std::memory_order_relaxed);
    stats.misses = cache.misses.load(std::memory_order_relaxed);
    {
        std::shared_lock lock(cache.mutex);
        stats.entries = cache.entries.size();
    }
    return stats;
}

} // namespace we::runtime::reflection
