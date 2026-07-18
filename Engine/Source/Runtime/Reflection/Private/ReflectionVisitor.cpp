#include "Reflection/IReflectionVisitor.h"
#include "Reflection/ITypeRegistry.h"

namespace we::runtime::reflection {

std::size_t VisitTypeHierarchy(
    const ITypeRegistry& registry,
    TypeId typeId,
    IReflectionVisitor& visitor,
    const VisitOptions& options)
{
    std::size_t visits = 0;

    std::size_t ancestorCount = 0;
    const TypeId* ancestors = options.includeBases
        ? registry.GetAncestorChain(typeId, ancestorCount)
        : nullptr;

    auto visitOne = [&](TypeId id) -> bool {
        const TypeInfo* info = registry.Find(id);
        if (!info) {
            return true;
        }
        if (!visitor.VisitType(*info)) {
            return false;
        }
        ++visits;
        if (options.visitProperties) {
            for (const PropertyInfo& property : info->properties) {
                if (options.serializedPropertiesOnly && !property.IsSerialized()) {
                    continue;
                }
                if (!visitor.VisitProperty(*info, property)) {
                    return false;
                }
                ++visits;
            }
        }
        if (options.visitFunctions) {
            for (const FunctionInfo& function : info->functions) {
                if (!visitor.VisitFunction(*info, function)) {
                    return false;
                }
                ++visits;
            }
        }
        return true;
    };

    if (ancestors && ancestorCount > 0) {
        for (std::size_t i = 0; i < ancestorCount; ++i) {
            if (!visitOne(ancestors[i])) {
                break;
            }
        }
    } else {
        visitOne(typeId);
    }
    return visits;
}

} // namespace we::runtime::reflection
