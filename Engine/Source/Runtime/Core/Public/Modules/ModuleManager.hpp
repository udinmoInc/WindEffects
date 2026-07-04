#pragma once

#include "Core/Export.hpp"
#include "Modules/IModuleInterface.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

class CORE_API ModuleManager
{
public:
    static ModuleManager& Get();

    /**
     * Loads a module dynamically by name (loads the DLL).
     * @param ModuleName The name of the module (e.g. "WindEffects-ContentBrowser")
     * @return Pointer to the loaded module interface.
     */
    IModuleInterface* LoadModule(const std::string& ModuleName);

    /**
     * Unloads all loaded modules in reverse order of loading.
     */
    void UnloadAllModules();

private:
    ModuleManager() = default;
    ~ModuleManager();

    struct ModuleData
    {
        void* Handle; // OS specific DLL handle
        IModuleInterface* Interface;
    };

    std::unordered_map<std::string, ModuleData> LoadedModules;
    std::vector<std::string> LoadOrder;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
