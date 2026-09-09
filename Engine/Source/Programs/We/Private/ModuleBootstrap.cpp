// ==============================================================================
// WindEffects — We — ModuleBootstrap
// Internal implementation for the We module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>

#include "Core/BuildPaths.h"

namespace {

constexpr const char* kFeatureModuleDlls[] = {
    "WEAssetImporter.dll",
    "WEAssetProcessors.dll",
    "WEAssetPipeline.dll",
    "WEAssetCooker.dll",
    "WEAssetTools.dll",
    "WEText.dll",
    "WEIcons.dll",
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

void PreloadFeatureModules() {
    we::core::ConfigureModuleSearchPaths();
    for (const char* dllName : kFeatureModuleDlls) {
        LoadResolvedDelayLoadDll(dllName);
    }
}

struct WeModuleBootstrap {
    WeModuleBootstrap() {
        PreloadFeatureModules();
    }
};

#pragma init_seg(lib)
WeModuleBootstrap g_WeModuleBootstrap;

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
