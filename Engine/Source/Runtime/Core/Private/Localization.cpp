// ==============================================================================
// WindEffects — Core — Localization
// Internal implementation for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/Localization.h"

namespace we::core {

Localization& Localization::Get() {
    static Localization instance;
    return instance;
}

void Localization::LoadStrings(const std::unordered_map<std::string, std::string>& dictionary) {
    for (const auto& [key, val] : dictionary) {
        m_Strings[key] = val;
    }
}

std::string_view Localization::GetString(std::string_view key, std::string_view defaultVal) const {
    auto it = m_Strings.find(std::string(key));
    if (it != m_Strings.end()) {
        return it->second;
    }
    return defaultVal.empty() ? key : defaultVal;
}

} // namespace we::core
