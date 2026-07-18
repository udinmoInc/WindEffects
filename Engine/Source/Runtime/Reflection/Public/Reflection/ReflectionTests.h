#pragma once

#include "Reflection/ReflectionProductionReport.h"

namespace we::runtime::reflection {

/// Alias for tooling that prefers a Tests-named entry point.
[[nodiscard]] inline ReflectionTestReport RunReflectionHardeningTests() {
    return RunReflectionTests();
}

} // namespace we::runtime::reflection
