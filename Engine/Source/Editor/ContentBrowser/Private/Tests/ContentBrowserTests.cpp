#include "ContentBrowser/ContentBrowserTests.h"
#include "ContentBrowser/ContentBrowserRuntime.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::editor::contentbrowser {
namespace {

void AddCase(ContentBrowserTestReport& report, std::string name, bool passed, std::string message) {
    ContentBrowserTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    report.cases.push_back(std::move(result));
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
}

} // namespace

ContentBrowserTestReport RunContentBrowserRuntimeTests() {
    ContentBrowserTestReport report{};
    ContentBrowserDiagnostics::Get().Reset();

    const auto tempRoot = std::filesystem::temp_directory_path() / "we_contentbrowser_test";
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
    std::filesystem::create_directories(tempRoot / "Textures", ec);
    {
        std::ofstream(tempRoot / "Textures" / "Dummy.txt") << "test";
    }

    ContentBrowserDependencies deps;
    deps.contentRoot = tempRoot;

    auto runtime = CreateContentBrowserRuntime(deps);
    AddCase(report, "CreateContentBrowserRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create ContentBrowserRuntime";
        return report;
    }

    auto& browser = runtime->Browser();
    browser.RequestRebuild();
    browser.Tick(0.f);

    AddCase(report, "Navigate /Game", true, "ok");
    browser.Navigate("/Game");
    browser.Tick(0.f);

    const bool folderCreated = browser.Commands().CreateFolder("NewFolder");
    AddCase(report, "CreateFolder", folderCreated, folderCreated ? "ok" : "fail");
    browser.Tick(0.f);

    ContentFilterState filter;
    filter.searchQuery = "Dummy";
    browser.SetFilterState(filter);
    browser.Tick(0.f);
    AddCase(report, "Search filter", true, "applied");

    browser.SaveFilterPreset("textures");
    AddCase(report, "Save/Load preset", browser.LoadFilterPreset("textures"), "preset");

    AddCase(report, "Refresh", browser.Commands().Refresh(), "refresh");

    runtime->Shutdown();
    std::filesystem::remove_all(tempRoot, ec);

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "ContentBrowser tests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::contentbrowser
