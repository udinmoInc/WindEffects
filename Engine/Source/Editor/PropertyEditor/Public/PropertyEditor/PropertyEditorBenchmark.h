// ==============================================================================
// WindEffects — PropertyEditor — PropertyEditorBenchmark
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "PropertyEditor/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::property {

struct PROPERTYEDITOR_API PropertyEditorBenchmarkSample {
    std::string name;
    double milliseconds = 0.0;
    std::uint64_t iterations = 0;
    double perIterationNs = 0.0;
};

struct PROPERTYEDITOR_API PropertyEditorBenchmarkConfig {
    std::uint32_t propertyCount = 1000;
    std::uint32_t multiObjectCount = 8;
    std::uint32_t filterIterations = 100;
};

struct PROPERTYEDITOR_API PropertyEditorBenchmarkReport {
    std::vector<PropertyEditorBenchmarkSample> samples;
    bool success = false;
    std::string summary;
};

[[nodiscard]] PROPERTYEDITOR_API PropertyEditorBenchmarkReport RunPropertyEditorBenchmarks(
    const PropertyEditorBenchmarkConfig& config = {});

/// A/B Inspector section expand/collapse: legacy clear-all editors vs kept-editors path.
/// Logs `[SectionExpandBench]` lines and returns a short summary string.
[[nodiscard]] PROPERTYEDITOR_API std::string RunDetailsSectionExpandInteractionBenchmark();

} // namespace we::editor::property
