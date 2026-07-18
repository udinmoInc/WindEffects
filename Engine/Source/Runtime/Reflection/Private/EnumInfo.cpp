#include "Reflection/EnumInfo.h"

namespace we::runtime::reflection {

const EnumValueInfo* EnumInfo::FindByName(std::string_view name) const noexcept {
    for (const EnumValueInfo& value : values) {
        if (value.name == name) {
            return &value;
        }
    }
    return nullptr;
}

const EnumValueInfo* EnumInfo::FindByValue(std::int64_t value) const noexcept {
    for (const EnumValueInfo& entry : values) {
        if (entry.value == value) {
            return &entry;
        }
    }
    return nullptr;
}

std::string_view EnumInfo::NameForValue(std::int64_t value) const noexcept {
    if (const EnumValueInfo* entry = FindByValue(value)) {
        return entry->name;
    }
    return {};
}

} // namespace we::runtime::reflection
