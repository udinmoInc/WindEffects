#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/NameId.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

struct REFLECTION_API ParameterInfo {
    std::string name;
    NameId nameId = kInvalidNameId;
    TypeId typeId = kInvalidTypeId;
    std::uint32_t index = 0;
    bool isConst = false;
    bool isReference = false;
    bool isOut = false;
};

using FunctionInvokeFn = bool (*)(
    void* instance,
    const void* args,
    std::size_t argsSize,
    void* returnBuffer,
    std::size_t returnBufferSize);

struct REFLECTION_API FunctionInfo {
    NameId nameId = kInvalidNameId;
    TypeId ownerTypeId = kInvalidTypeId;
    TypeId returnTypeId = kInvalidTypeId;
    FunctionFlags flags = FunctionFlags::None;
    FunctionInvokeFn invoke = nullptr;
    std::uint32_t index = 0;
    std::string name;
    std::vector<ParameterInfo> parameters;
    AttributeBag attributes;

    [[nodiscard]] bool IsCallable() const noexcept { return invoke != nullptr; }
    [[nodiscard]] bool IsStatic() const noexcept { return HasFlag(flags, FunctionFlags::Static); }
};

} // namespace we::runtime::reflection
