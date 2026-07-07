#pragma once

#include <string>

class IModuleInterface
{
public:
    virtual ~IModuleInterface() = default;

    /**
     * Called when the module is loaded and ready to initialize.
     */
    virtual void StartupModule() = 0;

    /**
     * Called when the module is being unloaded.
     */
    virtual void ShutdownModule() = 0;
};

// Macro to define the export function for the module
#define IMPLEMENT_MODULE(ModuleClass, ModuleName) \
    extern "C" __declspec(dllexport) IModuleInterface* InitializeModule() \
    { \
        return new ModuleClass(); \
    }
