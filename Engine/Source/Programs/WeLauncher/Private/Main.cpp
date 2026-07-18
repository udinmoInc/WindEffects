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
#include "../../Windows/Resources/resource.h"
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

#if defined(_WIN32)
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#endif

    const auto windowResult = platform.CreateWindow({
        .title = "WindEffects Launcher",
        .width = 1440,
        .height = 920,
        .resizable = true,
        .borderless = true,
        .visible = true,
        .highDpi = true,
    });
    if (!windowResult) {
        HE_ERROR("[WeLauncher] Failed to create window.");
        return -1;
    }
    const we::platform::WindowId window = *windowResult;

    (void)platform.ApplyWindowChrome(window, {
        .roundedCorners = true,
        .borderColorRgb = 0x303030,
    });
#if defined(_WIN32)
    (void)platform.SetWindowIcon(window, IDI_ICON1);
#endif

    try {
        we::programs::welauncher::WeLauncherApp app(window);
        app.Run();
    } catch (const std::exception& e) {
        const std::string message = std::string("[WeLauncher] Exception: ") + e.what();
        HE_ERROR(message);
#if defined(_WIN32)
        MessageBoxA(nullptr, e.what(), "WindEffects Launcher", MB_OK | MB_ICONERROR);
#endif
    } catch (...) {
        HE_ERROR("[WeLauncher] Unknown exception.");
#if defined(_WIN32)
        MessageBoxA(nullptr, "Unknown exception during launcher startup.", "WindEffects Launcher", MB_OK | MB_ICONERROR);
#endif
    }

    (void)platform.DestroyWindow(window);
    we::platform::Platform::Shutdown();
    return 0;
}
