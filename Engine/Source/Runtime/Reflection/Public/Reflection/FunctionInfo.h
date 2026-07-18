#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

/// Parameter metadata for reflected functions.
struct REFLECTION_API ParameterInfo {
    std::string name;
    TypeId typeId = kInvalidTypeId;
    std::uint32_t index = 0;
    bool isConst = false;
    bool isReference = false;
    bool isOut = false;
};

/// Opaque invoke signature: (instance, argsBlob, argsSize, returnBlob, returnSize) -> success.
/// Callers pack arguments according to ParameterInfo layout. Designed for scripting /
/// editor call bridges without C++ RTTI.
using FunctionInvokeFn = bool (*)(
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer,
    std::size_t returnBufferSize);

struct REFLECTION_API FunctionInfo {
    std::string name;
    TypeId ownerTypeId = kInvalidTypeId;
    TypeId returnTypeId = kInvalidTypeId;
    std::vector<ParameterInfo> parameters;
    FunctionFlags flags = FunctionFlags::None;
    FunctionInvokeFn invoke = nullptr;
    AttributeBag attributes;
    std::uint32_t index = 0;

    [[nodiscard]] bool IsCallable() const noexcept { return invoke != nullptr; }
    [[nodiscard]] bool IsStatic() const noexcept { return HasFlag(flags, FunctionFlags::Static); }
};

} // namespace we::runtime::reflection
