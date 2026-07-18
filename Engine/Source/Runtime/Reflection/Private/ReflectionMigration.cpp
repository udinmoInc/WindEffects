#include "Reflection/ReflectionMigration.h"
#include "Reflection/ITypeRegistry.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace we::runtime::reflection {
namespace {

std::mutex& MigrationMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<TypeId, std::vector<TypeMigrationEntry>, TypeIdHash>& Migrations() {
    static std::unordered_map<TypeId, std::vector<TypeMigrationEntry>, TypeIdHash> map;
    return map;
}

std::mutex& RemapMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<TypeId, TypeId, TypeIdHash>& Remaps() {
    static std::unordered_map<TypeId, TypeId, TypeIdHash> map;
    return map;
}

std::mutex& MismatchMutex() {
    static std::mutex mutex;
    return mutex;
}

SchemaMismatchHandler& MismatchHandler() {
    static SchemaMismatchHandler handler = nullptr;
    return handler;
}

void*& MismatchUserData() {
    static void* userData = nullptr;
    return userData;
}

} // namespace

void RegisterTypeMigration(TypeMigrationEntry entry) {
    if (entry.typeId == kInvalidTypeId || !entry.migrate) {
        return;
    }
    std::lock_guard lock(MigrationMutex());
    Migrations()[entry.typeId].push_back(entry);
}

void UnregisterTypeMigrations(TypeId typeId) {
    std::lock_guard lock(MigrationMutex());
    Migrations().erase(typeId);
}

void ClearTypeMigrations() {
    std::lock_guard lock(MigrationMutex());
    Migrations().clear();
}

bool MigrateInstance(
    TypeId typeId,
    void* instance,
    std::uint32_t fromSchemaVersion,
    std::uint32_t toSchemaVersion)
{
    if (!instance || typeId == kInvalidTypeId) {
        return false;
    }
    if (fromSchemaVersion == toSchemaVersion) {
        return true;
    }

    std::vector<TypeMigrationEntry> entries;
    {
        std::lock_guard lock(MigrationMutex());
        const auto it = Migrations().find(typeId);
        if (it != Migrations().end()) {
            entries = it->second;
        }
    }

    if (entries.empty()) {
        // No hooks registered — accept only identical versions (already handled) or reject.
        NotifySchemaMismatch(typeId, fromSchemaVersion, toSchemaVersion);
        return false;
    }

    bool any = false;
    for (const TypeMigrationEntry& entry : entries) {
        if (entry.minFromVersion != 0 && fromSchemaVersion < entry.minFromVersion) {
            continue;
        }
        if (entry.maxFromVersion != 0 && fromSchemaVersion > entry.maxFromVersion) {
            continue;
        }
        if (!entry.migrate(instance, fromSchemaVersion, toSchemaVersion, entry.userData)) {
            return false;
        }
        any = true;
    }
    if (!any) {
        NotifySchemaMismatch(typeId, fromSchemaVersion, toSchemaVersion);
        return false;
    }
    return true;
}

bool MigrateInstanceToCurrent(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    std::uint32_t fromSchemaVersion)
{
    const TypeInfo* info = registry.Find(typeId);
    if (!info) {
        return false;
    }
    return MigrateInstance(typeId, instance, fromSchemaVersion, info->versions.schemaVersion);
}

void RegisterTypeIdRemap(TypeId from, TypeId to) {
    if (from == kInvalidTypeId || to == kInvalidTypeId) {
        return;
    }
    std::lock_guard lock(RemapMutex());
    Remaps()[from] = to;
}

void ClearTypeIdRemaps() {
    std::lock_guard lock(RemapMutex());
    Remaps().clear();
}

TypeId RemapTypeId(TypeId id) {
    std::lock_guard lock(RemapMutex());
    const auto it = Remaps().find(id);
    return it == Remaps().end() ? id : it->second;
}

void SetSchemaMismatchHandler(SchemaMismatchHandler handler, void* userData) {
    std::lock_guard lock(MismatchMutex());
    MismatchHandler() = handler;
    MismatchUserData() = userData;
}

void NotifySchemaMismatch(
    TypeId typeId,
    std::uint32_t archivedSchema,
    std::uint32_t liveSchema)
{
    SchemaMismatchHandler handler = nullptr;
    void* userData = nullptr;
    {
        std::lock_guard lock(MismatchMutex());
        handler = MismatchHandler();
        userData = MismatchUserData();
    }
    if (handler) {
        handler(typeId, archivedSchema, liveSchema, userData);
    }
}

} // namespace we::runtime::reflection
