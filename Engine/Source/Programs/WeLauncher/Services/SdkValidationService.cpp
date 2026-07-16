#include "Services/SdkValidationService.h"

#include <cstdlib>
#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "Platform/UndefWin32Macros.h"
#endif

namespace we::programs::welauncher {
namespace {

bool CommandExistsOnPath(const char* command) {
#if defined(_WIN32)
    char buffer[MAX_PATH]{};
    const DWORD found = SearchPathA(nullptr, command, nullptr, MAX_PATH, buffer, nullptr);
    return found > 0;
#else
    (void)command;
    return false;
#endif
}

bool PathExists(const std::filesystem::path& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

} // namespace

std::vector<SdkCheckResult> SdkValidationService::RunChecks() const {
    std::vector<SdkCheckResult> results;

    {
        SdkCheckResult item{};
        item.name = "MSVC Toolchain";
        if (CommandExistsOnPath("cl.exe")) {
            item.status = SdkCheckStatus::Pass;
            item.detail = "cl.exe found on PATH";
        } else {
            item.status = SdkCheckStatus::Warn;
            item.detail = "cl.exe not on PATH — run from a VS developer shell or install MSVC";
        }
        results.push_back(item);
    }

    {
        SdkCheckResult item{};
        item.name = "Windows SDK";
#if defined(_WIN32)
        const char* kits = std::getenv("WindowsSdkDir");
        if (kits && PathExists(kits)) {
            item.status = SdkCheckStatus::Pass;
            item.detail = kits;
        } else if (PathExists("C:/Program Files (x86)/Windows Kits/10")) {
            item.status = SdkCheckStatus::Pass;
            item.detail = "Windows Kits 10 detected";
        } else {
            item.status = SdkCheckStatus::Warn;
            item.detail = "Windows SDK not detected";
        }
#else
        item.status = SdkCheckStatus::Pass;
        item.detail = "Not required on this platform";
#endif
        results.push_back(item);
    }

    {
        SdkCheckResult item{};
        item.name = "Vulkan SDK";
        const char* vulkan = std::getenv("VULKAN_SDK");
        if (vulkan && PathExists(vulkan)) {
            item.status = SdkCheckStatus::Pass;
            item.detail = vulkan;
        } else if (PathExists("Engine/ThirdParty/Vulkan")) {
            item.status = SdkCheckStatus::Pass;
            item.detail = "Bundled Vulkan headers/libs";
        } else {
            item.status = SdkCheckStatus::Warn;
            item.detail = "Vulkan SDK optional for launcher UI";
        }
        results.push_back(item);
    }

    {
        SdkCheckResult item{};
        item.name = ".NET (IgniteBT)";
        if (CommandExistsOnPath("dotnet.exe")) {
            item.status = SdkCheckStatus::Pass;
            item.detail = "dotnet available for build tooling";
        } else {
            item.status = SdkCheckStatus::Warn;
            item.detail = "dotnet not found — IgniteBT bootstrap may be limited";
        }
        results.push_back(item);
    }

    return results;
}

} // namespace we::programs::welauncher
