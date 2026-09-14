// ==============================================================================
// WindEffects — WeLauncher — LauncherTheme
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Theme/GraphiteDarkTheme.h"
#include "KindUI/Theme/DesignToken.h"

namespace we::programs::welauncher {

/// Launcher product theme — GraphiteDark colors + launcher metric overrides.
class LauncherTheme final : public we::runtime::kindui::GraphiteDarkTheme {
public:
    [[nodiscard]] std::string_view GetThemeId() const override { return "Launcher"; }

    [[nodiscard]] float ResolveMetric(we::runtime::kindui::MetricToken token) const override;
};

} // namespace we::programs::welauncher
