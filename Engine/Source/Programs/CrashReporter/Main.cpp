#include "Platform/PlatformSDK.h"
#include "Core/Logger.h"
#include "Core/BuildPaths.h"
#include "CrashReporterApp.h"
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
#include "../Windows/resource.h"
#endif

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    HE_INFO("[CrashReporter] === Starting WeCrashReporter.exe ===");

    we::runtime::core::Logger::Init();
    auto& platform = we::platform::Platform::Initialize({
        .appName = "WindEffects Crash Reporter",
        .highDpiAware = true,
        .enableDiagnostics = true,
    });

    const std::string exeDir = platform.GetExecutableDirectory();
    if (!exeDir.empty()) {
        platform.SetCurrentWorkingDirectory(exeDir);
        HE_INFO("[CrashReporter] Working directory set to: " + exeDir);
    }

    we::core::ConfigureModuleSearchPaths();

    const auto windowResult = platform.CreateWindow({
        .title = "WindEffects Crash Reporter",
        .width = 1200,
        .height = 760,
        .resizable = true,
        .visible = true,
        .highDpi = true,
    });
    if (!windowResult) {
        HE_ERROR("[CrashReporter] Failed to create platform window: " + windowResult.error.message);
        return -1;
    }
    const we::platform::WindowId window = *windowResult;

#if defined(_WIN32)
    (void)platform.SetWindowIcon(window, IDI_ICON1);
#endif

    try {
        we::programs::crashreporter::CrashReporterApp app(window);
        app.Run();
    } catch (const std::exception& e) {
        HE_ERROR("[CrashReporter] Exception in CrashReporterApp: " + std::string(e.what()));
    } catch (...) {
        HE_ERROR("[CrashReporter] Unknown exception in CrashReporterApp");
    }

    (void)platform.DestroyWindow(window);
    we::platform::Platform::Shutdown();
    HE_INFO("[CrashReporter] === WeCrashReporter.exe exiting ===");
    return 0;
}
