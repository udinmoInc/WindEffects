// ==============================================================================
// WindEffects — Reflection — FunctionInvoke
// Internal implementation for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Reflection/FunctionInvoke.h"
#include "Reflection/ITypeRegistry.h"

namespace we::runtime::reflection {

bool InvokeFunction(
    const FunctionInfo& function,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer,
    std::size_t returnBufferSize)
{
    if (!function.IsCallable()) {
        return false;
    }
    if (!function.IsStatic() && !instance) {
        return false;
    }
    return function.invoke(instance, args, argsSize, returnBuffer, returnBufferSize);
}

bool InvokeFunctionByName(
    const ITypeRegistry& registry,
    TypeId ownerTypeId,
    std::string_view functionName,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer,
    std::size_t returnBufferSize)
{
    const FunctionInfo* function = registry.FindFunction(ownerTypeId, functionName);
    if (!function) {
        return false;
    }
    return InvokeFunction(*function, instance, args, argsSize, returnBuffer, returnBufferSize);
}

bool InvokeFunctionByNameId(
    const ITypeRegistry& registry,
    TypeId ownerTypeId,
    NameId functionNameId,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer,
    std::size_t returnBufferSize)
{
    const FunctionInfo* function = registry.FindFunction(ownerTypeId, functionNameId);
    if (!function) {
        return false;
    }
    return InvokeFunction(*function, instance, args, argsSize, returnBuffer, returnBufferSize);
}

} // namespace we::runtime::reflection
