#include "Reflection/PropertyIterator.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/NameId.h"

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

} // namespace

std::vector<PropertyVisit> CollectProperties(
    const ITypeRegistry& registry,
    TypeId typeId,
    const PropertyIterateOptions& options)
{
    std::vector<PropertyVisit> out;

    // Prefer ancestor chain cache — no recursive allocations.
    std::size_t ancestorCount = 0;
    const TypeId* ancestors = nullptr;
    if (options.includeBases) {
        ancestors = registry.GetAncestorChain(typeId, ancestorCount);
    }

    std::unordered_set<NameId> seen;
    auto appendFromType = [&](TypeId ownerId) {
        const TypeInfo* info = registry.Find(ownerId);
        if (!info) {
            return;
        }
        for (const PropertyInfo& property : info->properties) {
            if (!PassesFilter(property, options)) {
                continue;
            }
            if (property.nameId != kInvalidNameId && !seen.insert(property.nameId).second) {
                // Keep first (most-derived is first in ancestor chain).
                continue;
            }
            PropertyVisit visit;
            visit.declaringType = info;
            visit.property = &property;
            visit.instanceOffset = property.offset;
            out.push_back(visit);
        }
    };

    if (ancestors && ancestorCount > 0) {
        // ancestors[0] is self — emit self first then bases for override semantics via seen set.
        for (std::size_t i = 0; i < ancestorCount; ++i) {
            appendFromType(ancestors[i]);
        }
    } else {
        appendFromType(typeId);
    }
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
