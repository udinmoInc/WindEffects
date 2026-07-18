#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

/// Query filter for enumerating registered types.
struct REFLECTION_API TypeQuery {
    TypeKind kind = TypeKind::Unknown; // Unknown = any
    TypeId baseTypeId = kInvalidTypeId;
    TypeId interfaceTypeId = kInvalidTypeId;
    bool instantiableOnly = false;
};

/// Thread-safe, deterministic type metadata registry.
/// This is the engine's single source of truth for runtime type information.
class REFLECTION_API ITypeRegistry {
public:
    virtual ~ITypeRegistry() = default;

    /// Register or replace a type. Registration order within a module should be deterministic.
    /// Returns false if typeId is invalid or name is empty.
    virtual bool RegisterType(TypeInfo info) = 0;

    /// Unregister by TypeId. Returns false if not found.
    virtual bool UnregisterType(TypeId typeId) = 0;

    /// Clear all types (including builtins unless preserveBuiltins is true).
    virtual void Clear(bool preserveBuiltins = true) = 0;

    [[nodiscard]] virtual const TypeInfo* Find(TypeId typeId) const = 0;
    [[nodiscard]] virtual const TypeInfo* FindByName(std::string_view qualifiedName) const = 0;
    [[nodiscard]] virtual const TypeInfo* FindByShortName(std::string_view shortName) const = 0;

    [[nodiscard]] virtual bool Contains(TypeId typeId) const = 0;
    [[nodiscard]] virtual std::size_t GetTypeCount() const = 0;
    [[nodiscard]] virtual std::vector<TypeId> GetAllTypeIds() const = 0;
    [[nodiscard]] virtual std::vector<const TypeInfo*> Query(const TypeQuery& query) const = 0;

    /// Resolve alias chains to the concrete type.
    [[nodiscard]] virtual const TypeInfo* Resolve(TypeId typeId) const = 0;

    /// Walk inheritance: returns true if typeId is-a candidateBase (including self).
    [[nodiscard]] virtual bool IsA(TypeId typeId, TypeId candidateBase) const = 0;

    /// Stable fingerprint of the entire registry (sorted type ids + schema versions).
    /// Useful for package / network schema negotiation.
    [[nodiscard]] virtual std::uint64_t ComputeFingerprint() const = 0;

    /// Reflection metadata format version for this registry instance.
    [[nodiscard]] virtual std::uint32_t GetSchemaVersion() const = 0;
};

struct REFLECTION_API TypeRegistryDependencies {
    bool registerBuiltins = true;
};

[[nodiscard]] REFLECTION_API std::unique_ptr<ITypeRegistry> CreateTypeRegistry(
    TypeRegistryDependencies deps = {});

/// Process-wide default registry (created on first use, registers builtins).
[[nodiscard]] REFLECTION_API ITypeRegistry& GetTypeRegistry();

} // namespace we::runtime::reflection
