// ==============================================================================
// WindEffects — Reflection — EnumInfo
// Public API surface for the Reflection module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
