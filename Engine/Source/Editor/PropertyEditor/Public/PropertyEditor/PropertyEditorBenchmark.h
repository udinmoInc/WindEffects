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

} // namespace we::editor::property
