#pragma once

#include "Serialization/Export.h"
#include "Serialization/Types.h"
#include "Reflection/ReflectionOps.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::serialization {

/// Binary delta between two serialized object payloads (or live instances via Reflection).
struct SERIALIZATION_API BinaryDelta {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::uint64_t baseFingerprint = 0;
    std::uint64_t targetFingerprint = 0;
    std::uint32_t schemaVersion = 1;
    reflection::ReflectionPatch patch{};
    std::vector<std::uint8_t> encoded; // packaged delta document bytes
};

struct SERIALIZATION_API BinaryDiffEntry {
    std::string path;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::vector<std::uint8_t> before;
    std::vector<std::uint8_t> after;
};

struct SERIALIZATION_API BinaryDiff {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::vector<BinaryDiffEntry> entries;
    bool identical = true;
};

struct SERIALIZATION_API Snapshot {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::uint64_t fingerprint = 0;
    std::uint32_t schemaVersion = 1;
    std::vector<std::uint8_t> bytes;
};

class ISerializer;

} // namespace we::runtime::serialization

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::serialization {

/// Build a Reflection-backed binary diff between two live instances.
[[nodiscard]] SERIALIZATION_API BinaryDiff DiffInstances(
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* left,
    const void* right,
    bool serializedOnly = true);

/// Generate a delta document (base → target) using Reflection patching.
[[nodiscard]] SERIALIZATION_API BinaryDelta GenerateDelta(
    ISerializer& serializer,
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* baseInstance,
    const void* targetInstance,
    const SerializeOptions& options = {});

/// Apply a delta onto a live base instance (mutates in place).
[[nodiscard]] SERIALIZATION_API bool ApplyDelta(
    const reflection::ITypeRegistry& registry,
    void* instance,
    const BinaryDelta& delta);

/// Capture a snapshot blob for incremental / undo workflows.
[[nodiscard]] SERIALIZATION_API Snapshot CaptureSnapshot(
    ISerializer& serializer,
    reflection::TypeId typeId,
    const void* instance,
    const SerializeOptions& options = {});

/// Restore a snapshot onto an instance.
[[nodiscard]] SERIALIZATION_API bool RestoreSnapshot(
    ISerializer& serializer,
    reflection::TypeId typeId,
    void* instance,
    const Snapshot& snapshot,
    const SerializeOptions& options = {});

/// Partial serialize: only include named property paths (dotted).
[[nodiscard]] SERIALIZATION_API std::vector<std::uint8_t> SerializePartial(
    ISerializer& serializer,
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* instance,
    const std::vector<std::string>& propertyPaths,
    const SerializeOptions& options = {});

} // namespace we::runtime::serialization
