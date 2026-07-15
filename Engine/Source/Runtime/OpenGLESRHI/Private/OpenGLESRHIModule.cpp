#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"
#include <memory>

#if 1

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateOpenGLESRHI() {
    return CreateUnsupportedRHI(RHIBackend::OpenGLES, "OpenGLES");
}
static RHIBackendRegistrar g_OpenGLESRegistrar(RHIBackend::OpenGLES, &CreateOpenGLESRHI, "OpenGLES");
} // namespace

class OpenGLESRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::OpenGLES, &CreateOpenGLESRHI, "OpenGLES");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::OpenGLESRHIModule, WindEffects_OpenGLESRHI)

#else

class OpenGLESRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {}
    void ShutdownModule() override {}
};

IMPLEMENT_MODULE(OpenGLESRHIModule, WindEffects_OpenGLESRHI)

#endif
