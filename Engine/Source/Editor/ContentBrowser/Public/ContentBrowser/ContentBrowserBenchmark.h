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
