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

    std::string LibraryName = ModuleName;
#ifdef _WIN32
    LibraryName += ".dll";
    void* Handle = LoadLibraryA(LibraryName.c_str());
#else
    LibraryName = "lib" + LibraryName + ".so";
    void* Handle = dlopen(LibraryName.c_str(), RTLD_NOW);
#endif

    if (!Handle)
    {
        std::cerr << "Failed to load module: " << LibraryName << std::endl;
        return nullptr;
    }

#ifdef _WIN32
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)GetProcAddress((HMODULE)Handle, "InitializeModule");
#else
    InitializeModuleFunc InitFunc = (InitializeModuleFunc)dlsym(Handle, "InitializeModule");
#endif

    if (!InitFunc)
    {
        std::cerr << "Failed to find InitializeModule in: " << LibraryName << std::endl;
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
