#pragma once

#include "Reflection/Export.h"
#include "Reflection/FunctionInfo.h"
#include "Reflection/NameId.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Generic runtime function invocation via reflected FunctionInfo.
[[nodiscard]] REFLECTION_API bool InvokeFunction(
    const FunctionInfo& function,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer = nullptr,
    std::size_t returnBufferSize = 0);

/// Lookup + invoke by name (walks inheritance). Static methods may pass instance=nullptr.
[[nodiscard]] REFLECTION_API bool InvokeFunctionByName(
    const ITypeRegistry& registry,
    TypeId ownerTypeId,
    std::string_view functionName,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer = nullptr,
    std::size_t returnBufferSize = 0);

[[nodiscard]] REFLECTION_API bool InvokeFunctionByNameId(
    const ITypeRegistry& registry,
    TypeId ownerTypeId,
    NameId functionNameId,
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer = nullptr,
    std::size_t returnBufferSize = 0);

} // namespace we::runtime::reflection
