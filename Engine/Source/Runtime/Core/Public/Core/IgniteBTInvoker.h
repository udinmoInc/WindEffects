// ==============================================================================
// WindEffects — Core — IgniteBTInvoker
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <string>
#include <vector>

namespace we::core {

struct IgniteBTInvokeResult {
    int exitCode = -1;
    bool launched = false;
    std::string errorMessage;
};

IgniteBTInvokeResult CORE_API InvokeIgniteBT(const std::vector<std::string>& args);
bool CORE_API TryResolveIgniteBTExecutable(std::string& outExecutablePath, std::string& outWorkingDirectory);

} // namespace we::core
