// ==============================================================================
// WindEffects — WeLauncher — LauncherLogo
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "RHI/Types.h"

#include <filesystem>
#include <string>

namespace we::runtime::kindui {
class OverlayRenderer;
}

namespace we::programs::welauncher {

// Loads Assets/Editor/Logo/Logo_UI.png into a UI descriptor set.
// Returns Invalid if the file is missing or upload fails.
[[nodiscard]] we::rhi::RHIDescriptorSetHandle LoadLauncherLogoTexture(
    we::runtime::kindui::OverlayRenderer* renderer,
    const std::filesystem::path& engineRoot,
    uint32_t displaySizePx);

[[nodiscard]] std::filesystem::path ResolveLauncherLogoPath(const std::filesystem::path& engineRoot);

} // namespace we::programs::welauncher
