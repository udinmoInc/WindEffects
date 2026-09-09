// ==============================================================================
// WindEffects — Core — Localization
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace we::core {

class Localization {
public:
    CORE_API static Localization& Get();

    CORE_API void LoadStrings(const std::unordered_map<std::string, std::string>& dictionary);
    [[nodiscard]] CORE_API std::string_view GetString(std::string_view key, std::string_view defaultVal = "") const;

private:
    Localization() = default;
    ~Localization() = default;

    std::unordered_map<std::string, std::string> m_Strings;
};

} // namespace we::core
