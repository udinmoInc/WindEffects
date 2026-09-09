// ==============================================================================
// WindEffects — Undo — UndoBenchmark
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Undo/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::undo {

struct UNDO_API UndoBenchmarkConfig {
    std::uint32_t propertyEdits = 5'000;
    std::uint32_t nestedDepth = 8;
    std::uint32_t historyDepth = 2'000;
    std::uint32_t multiObjectCount = 32;
};

struct UNDO_API UndoBenchmarkSample {
    std::string name;
    double milliseconds = 0.0;
    std::uint32_t iterations = 0;
};

struct UNDO_API UndoBenchmarkReport {
    std::vector<UndoBenchmarkSample> samples;
    bool success = false;
    std::string summary;
};

[[nodiscard]] UNDO_API UndoBenchmarkReport RunUndoRuntimeBenchmarks(
    const UndoBenchmarkConfig& config = {});

} // namespace we::editor::undo
