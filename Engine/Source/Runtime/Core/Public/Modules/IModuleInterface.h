#pragma once

namespace we::core {

class IModuleInterface {
public:
    virtual ~IModuleInterface() = default;

    virtual void StartupModule() = 0;

    // Defaulted so modules without shutdown work (RHI backends, etc.)
    // don't carry an empty override just to satisfy the base.
    virtual void ShutdownModule() {}
};

#define IMPLEMENT_MODULE(ModuleClass, ModuleName) \
    extern "C" __declspec(dllexport) we::core::IModuleInterface* InitializeModule() \
    { \
        return new ModuleClass(); \
    }

} // namespace we::core
