#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"
#include <memory>

#if _WIN32

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateDirectX11RHI() {
    return CreateUnsupportedRHI(RHIBackend::DirectX11, "DirectX11");
}
static RHIBackendRegistrar g_DirectX11Registrar(RHIBackend::DirectX11, &CreateDirectX11RHI, "DirectX11");
} // namespace

class DirectX11RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::DirectX11, &CreateDirectX11RHI, "DirectX11");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::DirectX11RHIModule, WindEffects_DirectX11RHI)

#else

class DirectX11RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {}
    void ShutdownModule() override {}
};

IMPLEMENT_MODULE(DirectX11RHIModule, WindEffects_DirectX11RHI)

#endif
