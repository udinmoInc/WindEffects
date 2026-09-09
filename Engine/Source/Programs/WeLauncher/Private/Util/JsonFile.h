// ==============================================================================
// WindEffects — WeLauncher — JsonFile
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.h>

namespace we::programs::welauncher {

class JsonFile {
public:
    static bool Load(const std::filesystem::path& path, nlohmann::json& out);
    static bool Save(const std::filesystem::path& path, const nlohmann::json& data);
    static std::optional<nlohmann::json> TryLoad(const std::filesystem::path& path);
};

} // namespace we::programs::welauncher
