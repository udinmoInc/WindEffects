#pragma once

#include "Reflection/Export.h"
#include "Reflection/NameId.h"
#include "Reflection/SerializePlan.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

struct REFLECTION_API TypeQuery {
    TypeKind kind = TypeKind::Unknown;
    TypeId baseTypeId = kInvalidTypeId;
    TypeId interfaceTypeId = kInvalidTypeId;
    bool instantiableOnly = false;
};

enum class RegisterMode : std::uint8_t {
    Replace = 0,       // default — overwrite existing TypeId
    FailIfExists,      // reject duplicates
    KeepExisting       // no-op if already registered
};

/// Thread-safe type metadata registry with lock-free reads after Seal().
class REFLECTION_API ITypeRegistry {
public:
    virtual ~ITypeRegistry() = default;

    virtual bool RegisterType(TypeInfo info) = 0;
    virtual bool RegisterType(TypeInfo info, RegisterMode mode) = 0;
    virtual bool UnregisterType(TypeId typeId) = 0;
    virtual void Clear(bool preserveBuiltins = true) = 0;

    /// Freeze mutations and publish an immutable snapshot for lock-free concurrent reads.
    virtual void Seal() = 0;
    /// Re-open registration (e.g. plugin load/unload). Readers fall back to shared locking.
    virtual void Unseal() = 0;
    [[nodiscard]] virtual bool IsSealed() const = 0;

    [[nodiscard]] virtual const TypeInfo* Find(TypeId typeId) const = 0;
    [[nodiscard]] virtual const TypeInfo* FindByName(std::string_view qualifiedName) const = 0;
    [[nodiscard]] virtual const TypeInfo* FindByShortName(std::string_view shortName) const = 0;
    [[nodiscard]] virtual const TypeInfo* FindByNameId(NameId qualifiedNameId) const = 0;

    [[nodiscard]] virtual const PropertyInfo* FindProperty(TypeId typeId, std::string_view name) const = 0;
    [[nodiscard]] virtual const PropertyInfo* FindProperty(TypeId typeId, NameId nameId) const = 0;
    [[nodiscard]] virtual const FunctionInfo* FindFunction(TypeId typeId, std::string_view name) const = 0;
    [[nodiscard]] virtual const FunctionInfo* FindFunction(TypeId typeId, NameId nameId) const = 0;

    /// Precomputed binary serialization plan (built at registration / seal time).
    [[nodiscard]] virtual const SerializePlan* GetSerializePlan(TypeId typeId) const = 0;

    /// Ancestor chain including self (stable pointers while sealed / until next mutation).
    [[nodiscard]] virtual const TypeId* GetAncestorChain(TypeId typeId, std::size_t& outCount) const = 0;
    [[nodiscard]] virtual const TypeId* GetDerivedTypes(TypeId typeId, std::size_t& outCount) const = 0;

    [[nodiscard]] virtual bool Contains(TypeId typeId) const = 0;
    [[nodiscard]] virtual std::size_t GetTypeCount() const = 0;
    [[nodiscard]] virtual std::vector<TypeId> GetAllTypeIds() const = 0;
    [[nodiscard]] virtual std::vector<const TypeInfo*> Query(const TypeQuery& query) const = 0;

    [[nodiscard]] virtual const TypeInfo* Resolve(TypeId typeId) const = 0;
    [[nodiscard]] virtual bool IsA(TypeId typeId, TypeId candidateBase) const = 0;
    [[nodiscard]] virtual std::uint64_t ComputeFingerprint() const = 0;
    [[nodiscard]] virtual std::uint32_t GetSchemaVersion() const = 0;

    /// Mark a type as referenced (clears "unused" diagnostic).
    virtual void MarkTypeUsed(TypeId typeId) = 0;

    /// Plugin-scoped registration: types registered until EndPluginRegistration carry the token.
    [[nodiscard]] virtual std::uint64_t BeginPluginRegistration(std::string_view pluginId) = 0;
    virtual void EndPluginRegistration() = 0;
    [[nodiscard]] virtual bool UnregisterByPluginToken(std::uint64_t token) = 0;

    /// Stable plugin id string recorded for a registration token (empty if unknown).
    [[nodiscard]] virtual std::string GetPluginId(std::uint64_t token) const = 0;
    /// TypeIds registered under the given plugin token (deterministic sorted order).
    [[nodiscard]] virtual std::vector<TypeId> GetTypesByPluginToken(std::uint64_t token) const = 0;
    /// Plugin token that owns a type (0 if builtin / unscoped).
    [[nodiscard]] virtual std::uint64_t GetOwningPluginToken(TypeId typeId) const = 0;
};

struct REFLECTION_API TypeRegistryDependencies {
    bool registerBuiltins = true;
    bool applyStaticInitializers = true;
    bool sealAfterBuiltins = false;
};

[[nodiscard]] REFLECTION_API std::unique_ptr<ITypeRegistry> CreateTypeRegistry(
    TypeRegistryDependencies deps = {});

[[nodiscard]] REFLECTION_API ITypeRegistry& GetTypeRegistry();

/// Compile-time / static initializer registration (runs once per process against a registry).
using StaticTypeInitFn = void (*)(ITypeRegistry&);
REFLECTION_API void RegisterStaticTypeInitializer(StaticTypeInitFn fn);
REFLECTION_API void ApplyStaticTypeInitializers(ITypeRegistry& registry);

} // namespace we::runtime::reflection
