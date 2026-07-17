#include "Modules/ModuleManager.h"

#include "Core/BuildPaths.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "WindEffects/ModuleInitializer.h"

#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace we::core {

namespace {

using InitializeModuleFunc = IModuleInterface* (*)();

bool IsRuntimeLinkedModule(std::string_view moduleName) {
    return moduleName == "WindEffects-KindUIFramework"
        || moduleName == "WindEffects-KindUI"
        || moduleName == "WindEffects_KindUI";
}

} // namespace

ModuleManager& ModuleManager::Get() {
    static ModuleManager instance;
    return instance;
}

ModuleManager::~ModuleManager() {
    UnloadAllModules();
}

IModuleInterface* ModuleManager::LoadModule(const std::string& moduleName) {
    if (const auto it = m_LoadedModules.find(moduleName); it != m_LoadedModules.end()) {
        return it->second.interface;
    }

    std::string loadedLibraryName;
    void* handle = nullptr;

#ifdef _WIN32
    if (const auto modulePath = ResolveModuleLibraryPath(moduleName)) {
        handle = LoadLibraryExW(
            modulePath->wstring().c_str(),
            nullptr,
            LOAD_WITH_ALTERED_SEARCH_PATH);
        if (handle) {
            loadedLibraryName = modulePath->string();
        }
    }

    if (!handle && IsRuntimeLinkedModule(moduleName)) {
        const auto candidates = BuildModuleBinaryCandidates(StripLegacyModulePrefix(moduleName));
        for (const auto& libName : candidates) {
            handle = LoadLibraryA(libName.c_str());
            if (handle) {
                loadedLibraryName = libName;
                break;
            }
        }
    }
#else
    if (const auto modulePath = ResolveModuleLibraryPath(moduleName)) {
        handle = dlopen(modulePath->c_str(), RTLD_NOW);
        if (handle) {
            loadedLibraryName = modulePath->string();
        }
    } else if (IsRuntimeLinkedModule(moduleName)) {
        const auto candidates = BuildModuleBinaryCandidates(StripLegacyModulePrefix(moduleName));
        for (const auto& libName : candidates) {
            const std::string prefixed = "lib" + libName;
            handle = dlopen(prefixed.c_str(), RTLD_NOW);
            if (handle) {
                loadedLibraryName = prefixed;
                break;
            }
        }
    }
#endif

    if (!handle) {
        WE_LOG_ERROR(LogCategory::Build.data(), "Failed to load module: " + moduleName);
        return nullptr;
    }

#ifdef _WIN32
    const auto initFunc = reinterpret_cast<InitializeModuleFunc>(
        GetProcAddress(static_cast<HMODULE>(handle), "InitializeModule"));
#else
    const auto initFunc = reinterpret_cast<InitializeModuleFunc>(
        dlsym(handle, "InitializeModule"));
#endif

    if (!initFunc) {
        WE_LOG_ERROR(LogCategory::Build.data(), "Failed to find InitializeModule in: " + loadedLibraryName);
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        return nullptr;
    }

    IModuleInterface* moduleInterface = initFunc();
    if (!moduleInterface) {
        return nullptr;
    }

    m_LoadedModules[moduleName] = {handle, moduleInterface};
    m_LoadOrder.push_back(moduleName);
    moduleInterface->StartupModule();
    ModuleInitializerRegistry::Get().RunStartup(moduleName);
    return moduleInterface;
}

void ModuleManager::UnloadAllModules() {
    for (auto it = m_LoadOrder.rbegin(); it != m_LoadOrder.rend(); ++it) {
        const std::string& moduleName = *it;
        ModuleData& data = m_LoadedModules[moduleName];

        if (data.interface) {
            ModuleInitializerRegistry::Get().RunShutdown(moduleName);
            data.interface->ShutdownModule();
            delete data.interface;
        }

        if (data.handle) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(data.handle));
#else
            dlclose(data.handle);
#endif
        }
    }

    m_LoadedModules.clear();
    m_LoadOrder.clear();
}

} // namespace we::core
