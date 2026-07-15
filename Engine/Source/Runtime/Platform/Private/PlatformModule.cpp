#include "Modules/IModuleInterface.h"
#include "WindEffects/ModuleInitializer.h"

#include "Platform/Platform.h"

namespace we::runtime::platform {

class PlatformModule final : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        // Eagerly construct the host backend so clients can call Platform::Get() immediately.
        (void)we::platform::Platform::Get();
    }

    void ShutdownModule() override {
        we::platform::Platform::Shutdown();
    }
};

} // namespace we::runtime::platform

IMPLEMENT_MODULE(we::runtime::platform::PlatformModule, WindEffects_Platform)
