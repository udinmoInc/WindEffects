// ==============================================================================
// WindEffects — CrashReporter — Main
// Internal implementation for the CrashReporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Platform/PlatformSDK.h"
#include "Core/Logger.h"
#include "Core/BuildPaths.h"
#include "Core/ProductMetadata.h"
#include "Core/ExecutableMetadata.h"
#include "CrashReporterApp.h"
#include <filesystem>
#include <fstream>

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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
    HE_INFO("[CrashReporter] === Starting WeCrashReporter.exe ===");

    we::runtime::core::Logger::Init();

    we::core::ProductMetadataService::Get().Initialize();
    we::core::ExecutableMetadata::ConfigureAsCrashReporter();

    std::string crashDumpPath;
    std::string crashedAppMetadataPath;
    std::string cmdLine(lpCmdLine ? lpCmdLine : "");

    size_t dumpPos = cmdLine.find("--dump");
    if (dumpPos != std::string::npos) {
        size_t start = cmdLine.find_first_not_of(" \t", dumpPos + 6);
        if (start != std::string::npos) {
            size_t end = cmdLine.find_first_of(" \t", start);
            if (end == std::string::npos) end = cmdLine.length();
            crashDumpPath = cmdLine.substr(start, end - start);
        }
    }

    size_t metaPos = cmdLine.find("--metadata");
    if (metaPos != std::string::npos) {
        size_t start = cmdLine.find_first_not_of(" \t", metaPos + 10);
        if (start != std::string::npos) {
            size_t end = cmdLine.find_first_of(" \t", start);
            if (end == std::string::npos) end = cmdLine.length();
            crashedAppMetadataPath = cmdLine.substr(start, end - start);
        }
    }

    if (!crashedAppMetadataPath.empty()) {
        HE_INFO("[CrashReporter] Loading crashed application metadata from: " + crashedAppMetadataPath);
        std::ifstream metaFile(crashedAppMetadataPath);
        if (metaFile.is_open()) {
            std::string serialized((std::istreambuf_iterator<char>(metaFile)),
                                  std::istreambuf_iterator<char>());
            if (we::core::ProductMetadataService::Get().LoadFromSerialized(serialized)) {
                HE_INFO("[CrashReporter] Successfully loaded crashed application metadata");
                HE_INFO("[CrashReporter] Crashed app: " +
                        we::core::ProductMetadataService::Get().GetCrashedApplicationMetadata()->GetExecutableDisplayName());
            } else {
                HE_ERROR("[CrashReporter] Failed to parse crashed application metadata");
            }
        } else {
            HE_ERROR("[CrashReporter] Failed to open metadata file: " + crashedAppMetadataPath);
        }
    } else {
        HE_INFO("[CrashReporter] No crashed application metadata provided - running standalone");
    }

    std::string appName = we::core::ProductMetadataService::Get().GetMetadata().GetExecutableDisplayName();

    auto& platform = we::platform::Platform::Initialize({
        .appName = appName.c_str(),
        .highDpiAware = true,
        .enableDiagnostics = true,
    });

    const std::string exeDir = platform.GetExecutableDirectory();
    if (!exeDir.empty()) {
        platform.SetCurrentWorkingDirectory(exeDir);
        HE_INFO("[CrashReporter] Working directory set to: " + exeDir);
    }

    we::core::ConfigureModuleSearchPaths();

    std::string windowTitle = we::core::ProductMetadataService::Get().GetMetadata().GetExecutableDisplayName();

    const auto windowResult = platform.CreateWindow({
        .title = windowTitle.c_str(),
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
    we::runtime::core::Logger::Shutdown();
    return 0;
}
