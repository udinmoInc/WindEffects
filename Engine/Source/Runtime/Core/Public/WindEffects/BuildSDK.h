// ==============================================================================
// WindEffects — Core — BuildSDK
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects Build SDK — invoke IgniteBT from any C++ program.

#include "WindEffects/Platform.h"
#include "Core/IgniteBTInvoker.h"

#include <string>
#include <vector>

namespace we::build {

struct BuildRequest {
    std::vector<std::string> arguments;
};

inline we::core::IgniteBTInvokeResult CompileDebug() {
    return we::core::InvokeIgniteBT({"build", "--config", "Debug"});
}

inline we::core::IgniteBTInvokeResult CompileDevelopment() {
    return we::core::InvokeIgniteBT({"build", "--config", "Development"});
}

inline we::core::IgniteBTInvokeResult Run(const std::vector<std::string>& arguments) {
    return we::core::InvokeIgniteBT(arguments);
}

} // namespace we::build
