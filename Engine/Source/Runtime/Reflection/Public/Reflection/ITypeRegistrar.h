#pragma once

#include "Reflection/Export.h"
#include "Reflection/ITypeRegistry.h"

namespace we::runtime::reflection {

/// Plugin / module extension point for registering reflected types.
/// External modules implement this and call RegisterTypes during their StartupModule.
class REFLECTION_API ITypeRegistrar {
public:
    virtual ~ITypeRegistrar() = default;
    virtual void RegisterTypes(ITypeRegistry& registry) = 0;
    virtual void UnregisterTypes(ITypeRegistry& registry) = 0;
};

/// Applies a registrar against the process-wide registry.
REFLECTION_API void ApplyTypeRegistrar(ITypeRegistrar& registrar);
REFLECTION_API void RemoveTypeRegistrar(ITypeRegistrar& registrar);

} // namespace we::runtime::reflection
