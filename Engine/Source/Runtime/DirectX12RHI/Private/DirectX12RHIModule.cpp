#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"
#include <memory>

#if _WIN32

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateDirectX12RHI() {
    return CreateUnsupportedRHI(RHIBackend::DirectX12, "DirectX12");
}
static RHIBackendRegistrar g_DirectX12Registrar(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");
} // namespace

class DirectX12RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::DirectX12, &CreateDirectX12RHI, "DirectX12");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::DirectX12RHIModule, WindEffects_DirectX12RHI)

#else

class DirectX12RHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {}
    void ShutdownModule() override {}
};

IMPLEMENT_MODULE(DirectX12RHIModule, WindEffects_DirectX12RHI)

#endif
