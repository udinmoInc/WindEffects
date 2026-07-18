#pragma once

#include "Reflection/Export.h"
#include "Reflection/FunctionInfo.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"

#include <cstddef>
#include <cstdint>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Allocation-free reflection visitor. Prefer this over std::function visitors in hot paths.
class REFLECTION_API IReflectionVisitor {
public:
    virtual ~IReflectionVisitor() = default;

    /// Return false to stop traversal early.
    virtual bool VisitType(const TypeInfo& type) { (void)type; return true; }
    virtual bool VisitProperty(const TypeInfo& owner, const PropertyInfo& property) {
        (void)owner;
        (void)property;
        return true;
    }
    virtual bool VisitFunction(const TypeInfo& owner, const FunctionInfo& function) {
        (void)owner;
        (void)function;
        return true;
    }
};

struct REFLECTION_API VisitOptions {
    bool includeBases = true;
    bool visitProperties = true;
    bool visitFunctions = false;
    bool serializedPropertiesOnly = false;
};

/// Walk type (+ optional bases) invoking visitor. Returns number of Visit* calls that returned true.
[[nodiscard]] REFLECTION_API std::size_t VisitTypeHierarchy(
    const ITypeRegistry& registry,
    TypeId typeId,
    IReflectionVisitor& visitor,
    const VisitOptions& options = {});

} // namespace we::runtime::reflection
