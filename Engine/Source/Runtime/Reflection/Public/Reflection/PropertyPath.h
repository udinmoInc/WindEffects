#pragma once

#include "Reflection/Export.h"
#include "Reflection/NameId.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

/// One segment of a dotted property path (e.g. "transform.position.x").
struct REFLECTION_API PropertyPathSegment {
    NameId nameId = kInvalidNameId;
    std::string name;
};

/// Result of resolving a nested property path against a root type.
struct REFLECTION_API ResolvedPropertyPath {
    TypeId rootTypeId = kInvalidTypeId;
    TypeId leafTypeId = kInvalidTypeId;
    std::vector<PropertyPathSegment> segments;
    std::vector<const PropertyInfo*> properties; // parallel to segments; stable while sealed
    std::uint32_t absoluteOffset = 0;            // valid when allDirectStorage
    bool allDirectStorage = false;
    bool valid = false;

    [[nodiscard]] const PropertyInfo* Leaf() const noexcept {
        return properties.empty() ? nullptr : properties.back();
    }
};

/// Parse "a.b.c" into segments (no registry lookup). Empty on failure.
[[nodiscard]] REFLECTION_API std::vector<PropertyPathSegment> ParsePropertyPath(
    std::string_view path);

/// Resolve a dotted path relative to rootTypeId. Results may be cached internally.
[[nodiscard]] REFLECTION_API ResolvedPropertyPath ResolvePropertyPath(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    std::string_view path);

/// Resolve using pre-parsed / interned segment NameIds (zero string alloc on cache hit).
[[nodiscard]] REFLECTION_API ResolvedPropertyPath ResolvePropertyPath(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    const std::vector<NameId>& segmentNameIds);

/// Walk a resolved path to the leaf instance pointer (requires allDirectStorage).
[[nodiscard]] REFLECTION_API void* ResolvePropertyPathInstance(
    void* rootInstance,
    const ResolvedPropertyPath& path);

[[nodiscard]] REFLECTION_API const void* ResolvePropertyPathInstance(
    const void* rootInstance,
    const ResolvedPropertyPath& path);

/// Generic get/set via dotted path (supports offset chains and leaf getters/setters).
[[nodiscard]] REFLECTION_API bool GetPropertyPathRaw(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    const void* instance,
    std::string_view path,
    void* outValue,
    std::size_t outSize);

[[nodiscard]] REFLECTION_API bool SetPropertyPathRaw(
    const ITypeRegistry& registry,
    TypeId rootTypeId,
    void* instance,
    std::string_view path,
    const void* value,
    std::size_t valueSize);

/// Invalidate process-local path resolution caches (call after Unseal / bulk unregister).
REFLECTION_API void ClearPropertyPathCache();

/// Cache statistics for diagnostics / production reports.
struct REFLECTION_API PropertyPathCacheStats {
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;
    std::size_t entries = 0;
};

[[nodiscard]] REFLECTION_API PropertyPathCacheStats GetPropertyPathCacheStats();

} // namespace we::runtime::reflection
