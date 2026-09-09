// ==============================================================================
// WindEffects — WeLauncher — LauncherMaintenance
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <string>

namespace we::programs::welauncher {

struct AssociationResult {
    bool ok = false;
    std::string message;
};

[[nodiscard]] AssociationResult AssociateProjectExtension(const std::string& extensionWithDot);
[[nodiscard]] bool OpenPathInExplorer(const std::string& pathUtf8);
[[nodiscard]] bool OpenUrl(const std::string& url);

} // namespace we::programs::welauncher
