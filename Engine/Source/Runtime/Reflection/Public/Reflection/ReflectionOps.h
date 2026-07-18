#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

/// One field-level difference between two instances of the same reflected type.
struct REFLECTION_API ReflectionDiffEntry {
    std::string path;           // dotted property path
    TypeId propertyTypeId = kInvalidTypeId;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct REFLECTION_API ReflectionDiff {
    TypeId typeId = kInvalidTypeId;
    std::vector<ReflectionDiffEntry> entries;
    bool identical = true;
};

struct REFLECTION_API ReflectionPatchEntry {
    std::string path;
    std::vector<std::uint8_t> bytes; // raw leaf value bytes
};

struct REFLECTION_API ReflectionPatch {
    TypeId typeId = kInvalidTypeId;
    std::uint32_t schemaVersion = 1;
    std::vector<ReflectionPatchEntry> entries;
};

/// Default-construct then copy-assign / memcpy via TypeOps when available.
[[nodiscard]] REFLECTION_API void* CloneObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* source);

/// Destroy an instance previously produced by CloneObject / factory Create.
REFLECTION_API void DestroyClonedObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance);

/// Copy source → dest (same type). Prefer TypeOps; fall back to property-wise POD copy.
[[nodiscard]] REFLECTION_API bool CopyObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* destination,
    const void* source);

/// Lexicographic / byte compare of serialized property payloads. Returns 0 if equal.
[[nodiscard]] REFLECTION_API int CompareObjects(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right);

[[nodiscard]] REFLECTION_API bool AreObjectsEqual(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right);

/// Diff two instances; only reports properties that differ (serialize-flagged by default).
[[nodiscard]] REFLECTION_API ReflectionDiff DiffObjects(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right,
    bool serializedOnly = true);

/// Build a patch from left→right differences.
[[nodiscard]] REFLECTION_API ReflectionPatch BuildPatch(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* from,
    const void* to,
    bool serializedOnly = true);

/// Apply a patch onto an instance (paths must resolve).
[[nodiscard]] REFLECTION_API bool ApplyPatch(
    const ITypeRegistry& registry,
    void* instance,
    const ReflectionPatch& patch);

/// Deterministic FNV-1a-64 over serialized property bytes (stable across runs).
[[nodiscard]] REFLECTION_API std::uint64_t HashObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* instance,
    bool serializedOnly = true);

/// Schema-level stable serialization identifier for a type (TypeId + versions).
[[nodiscard]] REFLECTION_API std::uint64_t ComputeStableSerializationId(
    const ITypeRegistry& registry,
    TypeId typeId);

} // namespace we::runtime::reflection
