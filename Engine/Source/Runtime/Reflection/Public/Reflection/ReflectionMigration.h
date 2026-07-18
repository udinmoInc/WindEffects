#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Per-type schema migration callback.
/// Called when deserializing data whose schemaVersion differs from the live type.
/// Return true on success. May rewrite instance contents in-place.
using TypeMigrationFn = bool (*)(
    void* instance,
    std::uint32_t fromSchemaVersion,
    std::uint32_t toSchemaVersion,
    void* userData);

struct REFLECTION_API TypeMigrationEntry {
    TypeId typeId = kInvalidTypeId;
    TypeMigrationFn migrate = nullptr;
    void* userData = nullptr;
    std::uint32_t minFromVersion = 0; // inclusive; 0 = any
    std::uint32_t maxFromVersion = 0; // inclusive; 0 = any
};

/// Register a migration hook for a TypeId (process-wide, thread-safe).
REFLECTION_API void RegisterTypeMigration(TypeMigrationEntry entry);

/// Remove all migration hooks for a TypeId.
REFLECTION_API void UnregisterTypeMigrations(TypeId typeId);

/// Clear all registered migration hooks (tests / shutdown).
REFLECTION_API void ClearTypeMigrations();

/// Run registered migrations to bring an instance from fromSchema → toSchema.
[[nodiscard]] REFLECTION_API bool MigrateInstance(
    TypeId typeId,
    void* instance,
    std::uint32_t fromSchemaVersion,
    std::uint32_t toSchemaVersion);

/// Convenience: migrate using the live type's schemaVersion as the target.
[[nodiscard]] REFLECTION_API bool MigrateInstanceToCurrent(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    std::uint32_t fromSchemaVersion);

/// Schema evolution helpers — remap TypeId aliases across package epochs.
struct REFLECTION_API TypeIdRemap {
    TypeId from = kInvalidTypeId;
    TypeId to = kInvalidTypeId;
};

REFLECTION_API void RegisterTypeIdRemap(TypeId from, TypeId to);
REFLECTION_API void ClearTypeIdRemaps();
[[nodiscard]] REFLECTION_API TypeId RemapTypeId(TypeId id);

/// Optional hook invoked when binary deserialize detects a schema version mismatch.
using SchemaMismatchHandler = void (*)(
    TypeId typeId,
    std::uint32_t archivedSchema,
    std::uint32_t liveSchema,
    void* userData);

REFLECTION_API void SetSchemaMismatchHandler(SchemaMismatchHandler handler, void* userData = nullptr);
REFLECTION_API void NotifySchemaMismatch(
    TypeId typeId,
    std::uint32_t archivedSchema,
    std::uint32_t liveSchema);

} // namespace we::runtime::reflection
