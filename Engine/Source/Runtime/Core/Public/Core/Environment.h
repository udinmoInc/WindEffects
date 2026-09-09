// ==============================================================================
// WindEffects — Core — Environment
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <optional>
#include <string>

namespace we::core {

/// Reads an environment variable. Returns nullopt when the variable is unset.
std::optional<std::string> GetEnvVar(const char* name);

} // namespace we::core
