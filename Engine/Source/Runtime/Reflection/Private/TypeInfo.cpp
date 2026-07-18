#include "Reflection/TypeInfo.h"

namespace we::runtime::reflection {

const PropertyInfo* TypeInfo::FindProperty(std::string_view propertyName) const noexcept {
    for (const PropertyInfo& property : properties) {
        if (property.name == propertyName) {
            return &property;
        }
    }
    return nullptr;
}

const FunctionInfo* TypeInfo::FindFunction(std::string_view functionName) const noexcept {
    for (const FunctionInfo& function : functions) {
        if (function.name == functionName) {
            return &function;
        }
    }
    return nullptr;
}

bool TypeInfo::IsA(TypeId candidateBaseOrSelf) const noexcept {
    if (candidateBaseOrSelf == kInvalidTypeId) {
        return false;
    }
    if (typeId == candidateBaseOrSelf) {
        return true;
    }
    for (TypeId base : baseTypes) {
        if (base == candidateBaseOrSelf) {
            return true;
        }
    }
    for (TypeId iface : interfaces) {
        if (iface == candidateBaseOrSelf) {
            return true;
        }
    }
    return false;
}

} // namespace we::runtime::reflection
