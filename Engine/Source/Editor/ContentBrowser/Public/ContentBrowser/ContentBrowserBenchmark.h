// ==============================================================================
// WindEffects — ContentBrowser — ContentBrowserBenchmark
// Public API surface for the ContentBrowser module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "ContentBrowser/Export.h"

#include <cstdint>
#include <string>

namespace we::editor::contentbrowser {

struct CONTENTBROWSER_API ContentBrowserBenchmarkReport {
    std::uint64_t assetCount = 0;
    std::uint64_t refreshIterations = 0;
    std::uint64_t refreshTotalMicros = 0;
    std::uint64_t searchIterations = 0;
    std::uint64_t searchTotalMicros = 0;
    std::string summary;
};

[[nodiscard]] CONTENTBROWSER_API ContentBrowserBenchmarkReport RunContentBrowserBenchmarks(
    std::uint32_t syntheticAssetCount = 2000,
    std::uint32_t iterations = 40);

} // namespace we::editor::contentbrowser
