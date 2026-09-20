// ==============================================================================
// WindEffects — TestViewport — ModuleBootstrap
// Preloads RHI backends (VulkanRHI) so the renderer backend registers with RHIFactory.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <delayimp.h>

#include "Core/BuildPaths.h"

namespace {

constexpr const char* kModuleDlls[] = {
    "WERHI.dll",
    "WENullRHI.dll",
    "WEVulkanRHI.dll",
    "WERenderer.dll",
};

HMODULE LoadResolvedDelayLoadDll(const char* dllName) {
    if (dllName == nullptr || dllName[0] == '\0') {
        return nullptr;
    }

    we::core::ConfigureModuleSearchPaths();

    const auto modulePath = we::core::ResolveDelayLoadLibraryPath(dllName);
    if (!modulePath.has_value()) {
        return nullptr;
    }

    return LoadLibraryExW(
        modulePath->wstring().c_str(),
        nullptr,
        LOAD_WITH_ALTERED_SEARCH_PATH);
}

void PreloadTestViewportModules() {
    we::core::ConfigureModuleSearchPaths();

    for (const char* dllName : kModuleDlls) {
        LoadResolvedDelayLoadDll(dllName);
    }
}

struct TestViewportModuleBootstrap {
    TestViewportModuleBootstrap() {
        PreloadTestViewportModules();
    }
};

#pragma init_seg(lib)
TestViewportModuleBootstrap g_TestViewportModuleBootstrap;

} // namespace

extern "C" {

FARPROC WINAPI DelayLoadNotify(unsigned reason, DelayLoadInfo* info) {
    if (reason != dliNotePreLoadLibrary || info == nullptr || info->szDll == nullptr) {
        return 0;
    }

    HMODULE module = LoadResolvedDelayLoadDll(info->szDll);
    if (module != nullptr) {
        return reinterpret_cast<FARPROC>(module);
    }

    return 0;
}

FARPROC WINAPI DelayLoadFailureHook(unsigned reason, DelayLoadInfo* info) {
    if (reason != dliFailLoadLib || info == nullptr || info->szDll == nullptr) {
        return 0;
    }

    HMODULE module = LoadResolvedDelayLoadDll(info->szDll);
    if (module != nullptr) {
        return reinterpret_cast<FARPROC>(module);
    }

    return 0;
}

const PfnDliHook __pfnDliNotifyHook2 = DelayLoadNotify;
const PfnDliHook __pfnDliFailureHook2 = DelayLoadFailureHook;

} // extern "C"

#endif
