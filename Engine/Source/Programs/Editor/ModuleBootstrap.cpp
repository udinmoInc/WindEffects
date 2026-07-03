#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>

#include "Core/BuildPaths.hpp"

namespace {

constexpr const char* kFeatureModuleDlls[] = {
    "WindEffects-MainFrame.dll",
    "WindEffects-Viewport.dll",
    "WindEffects-Docking.dll",
    "WindEffects-ContentBrowser.dll",
    "WindEffects-WorldOutliner.dll",
    "WindEffects-PropertyEditor.dll",
    "WindEffects-Toolbar.dll",
    "WindEffects-Menus.dll",
    "WindEffects-Details.dll",
    "WindEffects-ToolsPanel.dll",
    "WindEffects-PlaceActors.dll",
    "WindEffects-Environment.dll",
};

HMODULE LoadFeatureModuleDll(const char* dllName) {
    if (dllName == nullptr || dllName[0] == '\0') {
        return nullptr;
    }

    const auto modulePath = we::core::ResolveDelayLoadLibraryPath(dllName);
    if (!modulePath.has_value()) {
        return nullptr;
    }

    return LoadLibraryExW(
        modulePath->c_str(),
        nullptr,
        LOAD_WITH_ALTERED_SEARCH_PATH);
}

void PreloadFeatureModules() {
    for (const char* dllName : kFeatureModuleDlls) {
        LoadFeatureModuleDll(dllName);
    }
}

struct EditorModuleBootstrap {
    EditorModuleBootstrap() {
        PreloadFeatureModules();
    }
};

#pragma init_seg(lib)
EditorModuleBootstrap g_EditorModuleBootstrap;

} // namespace

extern "C" {

FARPROC WINAPI DelayLoadNotify(unsigned reason, DelayLoadInfo* info) {
    if (reason != dliNotePreLoadLibrary || info == nullptr || info->szDll == nullptr) {
        return 0;
    }

    HMODULE module = LoadFeatureModuleDll(info->szDll);
    if (module != nullptr) {
        return reinterpret_cast<FARPROC>(module);
    }

    return 0;
}

FARPROC WINAPI DelayLoadFailureHook(unsigned reason, DelayLoadInfo* info) {
    if (reason != dliFailLoadLib || info == nullptr || info->szDll == nullptr) {
        return 0;
    }

    HMODULE module = LoadFeatureModuleDll(info->szDll);
    if (module != nullptr) {
        return reinterpret_cast<FARPROC>(module);
    }

    return 0;
}

const PfnDliHook __pfnDliNotifyHook2 = DelayLoadNotify;
const PfnDliHook __pfnDliFailureHook2 = DelayLoadFailureHook;

} // extern "C"

#endif
