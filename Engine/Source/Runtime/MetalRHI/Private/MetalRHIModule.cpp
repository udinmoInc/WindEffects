#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"
#include <memory>

#if __APPLE__

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateMetalRHI() {
    return CreateUnsupportedRHI(RHIBackend::Metal, "Metal");
}
static RHIBackendRegistrar g_MetalRegistrar(RHIBackend::Metal, &CreateMetalRHI, "Metal");
} // namespace

class MetalRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::Metal, &CreateMetalRHI, "Metal");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::MetalRHIModule, WindEffects_MetalRHI)

#else

class MetalRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {}
    void ShutdownModule() override {}
};

IMPLEMENT_MODULE(MetalRHIModule, WindEffects_MetalRHI)

#endif
