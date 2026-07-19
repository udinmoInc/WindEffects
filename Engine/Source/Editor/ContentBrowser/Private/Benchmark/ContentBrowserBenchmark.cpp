#include "ContentBrowser/ContentBrowserBenchmark.h"
#include "ContentBrowser/ContentBrowserRuntime.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::editor::contentbrowser {

ContentBrowserBenchmarkReport RunContentBrowserBenchmarks(
    std::uint32_t syntheticAssetCount,
    std::uint32_t iterations)
{
    ContentBrowserBenchmarkReport report{};
    report.assetCount = syntheticAssetCount;

    const auto tempRoot = std::filesystem::temp_directory_path() / "we_contentbrowser_bench";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    std::filesystem::create_directories(tempRoot, ec);
    for (std::uint32_t i = 0; i < syntheticAssetCount; ++i) {
        std::ofstream(tempRoot / ("Asset" + std::to_string(i) + ".txt")) << i;
    }

    ContentBrowserDependencies deps;
    deps.contentRoot = tempRoot;
    auto runtime = CreateContentBrowserRuntime(deps);
    auto& browser = runtime->Browser();

    using clock = std::chrono::steady_clock;
    {
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            (void)browser.Commands().Refresh();
            browser.Tick(0.f);
        }
        const auto t1 = clock::now();
        report.refreshIterations = iterations;
        report.refreshTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    {
        ContentFilterState filter;
        const auto t0 = clock::now();
        for (std::uint32_t i = 0; i < iterations; ++i) {
            filter.searchQuery = "Asset" + std::to_string(i % 100);
            browser.SetFilterState(filter);
            browser.Tick(0.f);
        }
        const auto t1 = clock::now();
        report.searchIterations = iterations;
        report.searchTotalMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }

    runtime->Shutdown();
    std::filesystem::remove_all(tempRoot, ec);

    std::ostringstream oss;
    oss << "ContentBrowser bench assets=" << syntheticAssetCount
        << " refresh=" << report.refreshTotalMicros << "us/" << report.refreshIterations
        << " search=" << report.searchTotalMicros << "us/" << report.searchIterations;
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::contentbrowser
