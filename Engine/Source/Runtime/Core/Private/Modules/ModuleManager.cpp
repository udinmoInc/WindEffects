#include "Modules/ModuleManager.h"
#include "Core/BuildPaths.h"
#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include <filesystem>

// Dynamic editor module loader (exported via CORE_API).

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef IModuleInterface* (*InitializeModuleFunc)();

bool IsRuntimeLinkedModule(const std::string& moduleName) {
    return moduleName == "WindEffects-UIFramework";
}

ModuleManager& ModuleManager::Get()
{
    static ModuleManager instance;
    return instance;
}

ModuleManager::~ModuleManager()
{
    UnloadAllModules();
}

IModuleInterface* ModuleManager::LoadModule(const std::string& ModuleName)
{
    if (LoadedModules.find(ModuleName) != LoadedModules.end())
    {
        return LoadedModules[ModuleName].Interface;
    }

    std::string LoadedLibraryName;
    void* Handle = nullptr;

#ifdef _WIN32
    if (const auto modulePath = we::core::ResolveModuleLibraryPath(ModuleName)) {
        Handle = LoadLibraryExW(
            modulePath->wstring().c_str(),
            nullptr,
            LOAD_WITH_ALTERED_SEARCH_PATH);
        if (Handle) {
            LoadedLibraryName = modulePath->string();
        }
    }

    if (!Handle && IsRuntimeLinkedModule(ModuleName)) {
        const auto candidates = we::core::BuildModuleBinaryCandidates(
            we::core::StripLegacyModulePrefix(ModuleName));
        for (const auto& libName : candidates) {
            Handle = LoadLibraryA(libName.c_str());
            if (Handle) {
                LoadedLibraryName = libName;
                break;
            }
        }
    }
#else
    if (const auto modulePath = we::core::ResolveModuleLibraryPath(ModuleName)) {
        Handle = dlopen(modulePath->c_str(), RTLD_NOW);
        if (Handle) {
            LoadedLibraryName = modulePath->string();
        }
    } else if (IsRuntimeLinkedModule(ModuleName)) {
        const auto candidates = we::core::BuildModuleBinaryCandidates(
            we::core::StripLegacyModulePrefix(ModuleName));
        for (const auto& libName : candidates) {
            const std::string prefixed = "lib" + libName;
            Handle = dlopen(prefixed.c_str(), RTLD_NOW);
            if (Handle) {
                LoadedLibraryName = prefixed;
                break;
            }
        }
    }
#endif

    if (!Handle)
    {
        WE_LOG_ERROR(we::LogCategory::Build.data(), "Failed to load module: " + ModuleName);
        return nullptr;
    }

#ifdef _WIN32
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)GetProcAddress((HMODULE)Handle, "InitializeModule");
#else
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)dlsym(Handle, "InitializeModule");
#endif

    if (!InitFunc)
    {
        WE_LOG_ERROR(we::LogCategory::Build.data(), "Failed to find InitializeModule in: " + LoadedLibraryName);
#ifdef _WIN32
        FreeLibrary((HMODULE)Handle);
#else
        dlclose(Handle);
#endif
        return nullptr;
    }

    IModuleInterface* ModuleInterface = InitFunc();
    if (ModuleInterface)
    {
        ModuleData Data;
        Data.Handle = Handle;
        Data.Interface = ModuleInterface;
        
        LoadedModules[ModuleName] = Data;
        LoadOrder.push_back(ModuleName);
        
        ModuleInterface->StartupModule();
        return ModuleInterface;
    }

    return nullptr;
}

void ModuleManager::UnloadAllModules()
{
    for (auto it = LoadOrder.rbegin(); it != LoadOrder.rend(); ++it)
    {
        const std::string& ModuleName = *it;
        ModuleData& Data = LoadedModules[ModuleName];
        
        if (Data.Interface)
        {
            Data.Interface->ShutdownModule();
            delete Data.Interface;
        }

        if (Data.Handle)
        {
#ifdef _WIN32
            FreeLibrary((HMODULE)Data.Handle);
#else
            dlclose(Data.Handle);
#endif
        }
    }
    LoadedModules.clear();
    LoadOrder.clear();
}
