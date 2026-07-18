#include "Reflection/PropertyIterator.h"
#include "Reflection/ITypeRegistry.h"

#include <unordered_set>

namespace we::runtime::reflection {
namespace {

bool PassesFilter(const PropertyInfo& property, const PropertyIterateOptions& options) {
    if (options.serializedOnly && !property.IsSerialized()) {
        return false;
    }
    if (options.editableOnly && !HasFlag(property.flags, PropertyFlags::Editable)) {
        return false;
    }
    if (options.skipHidden && HasFlag(property.flags, PropertyFlags::Hidden)) {
        return false;
    }
    if (options.skipTransient && HasFlag(property.flags, PropertyFlags::Transient)) {
        return false;
    }
    return true;
}

void CollectRecursive(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyIterateOptions& options,
    std::unordered_set<TypeId, TypeIdHash>& visited,
    std::vector<PropertyVisit>& out)
{
    if (typeId == kInvalidTypeId || !visited.insert(typeId).second) {
        return;
    }
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info) {
        return;
    }

    if (options.includeBases) {
        // Bases first so derived properties can override by name at the end.
        for (TypeId baseId : info->baseTypes) {
            CollectRecursive(registry, baseId, options, visited, out);
        }
    }

    for (const PropertyInfo& property : info->properties) {
        if (!PassesFilter(property, options)) {
            continue;
        }
        PropertyVisit visit;
        visit.declaringType = info;
        visit.property = &property;
        visit.instanceOffset = property.offset;
        out.push_back(visit);
    }
}

} // namespace

std::vector<PropertyVisit> CollectProperties(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyIterateOptions& options)
{
    std::vector<PropertyVisit> out;
    std::unordered_set<TypeId, TypeIdHash> visited;
    CollectRecursive(registry, typeId, options, visited, out);
    return out;
}

std::size_t ForEachProperty(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyVisitorFn& visitor,
    const PropertyIterateOptions& options)
{
    if (!visitor) {
        return 0;
    }
    const std::vector<PropertyVisit> visits = CollectProperties(registry, typeId, options);
    for (const PropertyVisit& visit : visits) {
        visitor(visit);
    }
    return visits.size();
}

} // namespace we::runtime::reflection
