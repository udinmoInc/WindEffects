#pragma once

#include "Reflection/Export.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Options controlling which properties are visited.
struct REFLECTION_API PropertyIterateOptions {
    bool includeBases = true;
    bool serializedOnly = false;
    bool editableOnly = false;
    bool skipHidden = true;
    bool skipTransient = false;
};

/// One visited property, optionally resolved against an inheritance chain.
struct REFLECTION_API PropertyVisit {
    const TypeInfo* declaringType = nullptr;
    const PropertyInfo* property = nullptr;
    /// Byte offset relative to the most-derived instance (accounts for base layout
    /// only when bases are embedded at offset 0 — single-inheritance POD layout).
    std::uint32_t instanceOffset = 0;
};

/// Collect properties for a type (and optionally its bases) into a flat list.
[[nodiscard]] REFLECTION_API std::vector<PropertyVisit> CollectProperties(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyIterateOptions& options = {});

/// Invoke visitor for each matching property. Returns number of visits.
using PropertyVisitorFn = std::function<void(const PropertyVisit& visit)>;

[[nodiscard]] REFLECTION_API std::size_t ForEachProperty(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyVisitorFn& visitor,
    const PropertyIterateOptions& options = {});

} // namespace we::runtime::reflection
