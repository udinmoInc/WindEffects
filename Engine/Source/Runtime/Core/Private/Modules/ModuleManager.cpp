#include "Modules/ModuleManager.hpp"
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

typedef IModuleInterface* (*InitializeModuleFunc)();

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

    std::string BaseName = ModuleName;
    std::string LibName;
    std::string ModName;
    std::string LoadedLibraryName;

#ifdef _WIN32
    LibName = BaseName + ".dll";
    ModName = "Modules\\" + LibName;
    void* Handle = LoadLibraryExA(ModName.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (Handle) {
        LoadedLibraryName = ModName;
    } else {
        Handle = LoadLibraryA(LibName.c_str());
        if (Handle) LoadedLibraryName = LibName;
    }
#else
    LibName = "lib" + BaseName + ".so";
    ModName = "Modules/" + LibName;
    void* Handle = dlopen(ModName.c_str(), RTLD_NOW);
    if (Handle) {
        LoadedLibraryName = ModName;
    } else {
        Handle = dlopen(LibName.c_str(), RTLD_NOW);
        if (Handle) LoadedLibraryName = LibName;
    }
#endif

    if (!Handle)
    {
        std::cerr << "Failed to load module: " << ModuleName << " (searched Modules/ and root)" << std::endl;
        return nullptr;
    }

#ifdef _WIN32
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)GetProcAddress((HMODULE)Handle, "InitializeModule");
#else
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)dlsym(Handle, "InitializeModule");
#endif

    if (!InitFunc)
    {
        std::cerr << "Failed to find InitializeModule in: " << LoadedLibraryName << std::endl;
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
