#include "Platform/PlatformSDK.h"
#include "Core/Logger.h"
#include "Core/BuildPaths.h"

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

#include "App/WeLauncherApp.h"

#if defined(_WIN32)
#include "Platform/UndefWin32Macros.h"
#endif

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    we::runtime::core::Logger::Init();

    auto& platform = we::platform::Platform::Initialize({
        .appName = "WindEffects Launcher",
        .highDpiAware = true,
        .enableDiagnostics = true,
    });

    const std::string exeDir = platform.GetExecutableDirectory();
    if (!exeDir.empty()) {
        platform.SetCurrentWorkingDirectory(exeDir);
    }

    we::core::ConfigureModuleSearchPaths();

    const auto windowResult = platform.CreateWindow({
        .title = "WindEffects Launcher",
        .width = 1400,
        .height = 900,
        .resizable = true,
        .visible = true,
        .highDpi = true,
    });
    if (!windowResult) {
        HE_ERROR("[WeLauncher] Failed to create window.");
        return -1;
    }
    const we::platform::WindowId window = *windowResult;

#if defined(_WIN32)
    (void)platform.SetWindowIcon(window, IDI_ICON1);
#endif

    try {
        we::programs::welauncher::WeLauncherApp app(window);
        app.Run();
    } catch (const std::exception& e) {
        HE_ERROR(std::string("[WeLauncher] Exception: ") + e.what());
    } catch (...) {
        HE_ERROR("[WeLauncher] Unknown exception.");
    }

    (void)platform.DestroyWindow(window);
    we::platform::Platform::Shutdown();
    return 0;
}
