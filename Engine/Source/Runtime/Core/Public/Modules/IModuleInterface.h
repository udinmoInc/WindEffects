#pragma once

namespace we::core {

class IModuleInterface {
public:
    virtual ~IModuleInterface() = default;

    virtual void StartupModule() = 0;
    virtual void ShutdownModule() = 0;
};

#define IMPLEMENT_MODULE(ModuleClass, ModuleName) \
    extern "C" __declspec(dllexport) we::core::IModuleInterface* InitializeModule() \
    { \
        return new ModuleClass(); \
    }

} // namespace we::core
