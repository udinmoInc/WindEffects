#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <delayimp.h>

#include "Core/BuildPaths.h"

namespace {

constexpr const char* kFeatureModuleDlls[] = {
    "WERHI.dll",
    "WENullRHI.dll",
    "WEVulkanRHI.dll",
    "WEDX12RHI.dll",
    "WEUI.dll",
    "WEProjects.dll",
    "WEAssetImporter.dll",
    "WEAssetProcessors.dll",
    "WEAssetPipeline.dll",
    "WEAssetCooker.dll",
    "WEAssetRuntime.dll",
    "WEUIFramework.dll",
    "WEMainFrame.dll",
    "WEViewport.dll",
    "WEContentBrowser.dll",
    "WEWorldOutliner.dll",
    "WEPropertyEditor.dll",
    "WEToolbar.dll",
    "WEMenus.dll",
    "WEToolsPanel.dll",
    "WEPlaceActors.dll",
    "WEEnvironment.dll",
    "WETerrain.dll",
    "WETerrainEditor.dll",
    "WEECS.dll",
    "WEEditorGridRenderer.dll",
    "WERenderer.dll",
    "WEScene.dll",
    "WEText.dll",
    "WEWorld.dll",
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
