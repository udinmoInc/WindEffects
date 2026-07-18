#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

struct REFLECTION_API EnumValueInfo {
    std::string name;
    std::int64_t value = 0;
    AttributeBag attributes;
};

/// Enum type metadata. Underlying integer storage size is in TypeInfo::size.
struct REFLECTION_API EnumInfo {
    TypeId typeId = kInvalidTypeId;
    std::vector<EnumValueInfo> values;
    bool isFlags = false; // bitfield-style enum

    [[nodiscard]] const EnumValueInfo* FindByName(std::string_view name) const noexcept;
    [[nodiscard]] const EnumValueInfo* FindByValue(std::int64_t value) const noexcept;
    [[nodiscard]] std::string_view NameForValue(std::int64_t value) const noexcept;
};

} // namespace we::runtime::reflection
