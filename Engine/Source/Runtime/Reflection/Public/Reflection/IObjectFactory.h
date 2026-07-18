#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"

#include <memory>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Creates and destroys instances of reflected types via registered TypeOps.
class REFLECTION_API IObjectFactory {
public:
    virtual ~IObjectFactory() = default;

    /// Heap-allocate and default-construct. Caller owns the result; destroy with Destroy().
    [[nodiscard]] virtual void* Create(TypeId typeId) const = 0;

    /// Destroy a heap instance previously returned by Create().
    virtual void Destroy(TypeId typeId, void* instance) const = 0;

    /// Placement-construct into caller-owned memory (must be >= TypeInfo::size, aligned).
    [[nodiscard]] virtual bool PlacementNew(TypeId typeId, void* memory) const = 0;

    /// Placement-destroy without freeing memory.
    virtual void PlacementDelete(TypeId typeId, void* memory) const = 0;

    [[nodiscard]] virtual bool CanCreate(TypeId typeId) const = 0;
};

struct REFLECTION_API ObjectFactoryDependencies {
    ITypeRegistry* registry = nullptr; // required; not owned
};

[[nodiscard]] REFLECTION_API std::unique_ptr<IObjectFactory> CreateObjectFactory(
    ObjectFactoryDependencies deps);

} // namespace we::runtime::reflection
