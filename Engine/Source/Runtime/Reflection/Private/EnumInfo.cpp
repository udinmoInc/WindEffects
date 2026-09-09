// ==============================================================================
// WindEffects — Reflection — EnumInfo
// Internal implementation for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
