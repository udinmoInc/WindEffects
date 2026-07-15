#include "Modules/IModuleInterface.h"
#include "RHI/RHIFactory.h"
#include "RHI/UnsupportedRHI.h"
#include <memory>

#if 1

namespace we::rhi {
namespace {
std::unique_ptr<IRHI> CreateOpenGLRHI() {
    return CreateUnsupportedRHI(RHIBackend::OpenGL, "OpenGL");
}
static RHIBackendRegistrar g_OpenGLRegistrar(RHIBackend::OpenGL, &CreateOpenGLRHI, "OpenGL");
} // namespace

class OpenGLRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {
        RHIFactory::Register(RHIBackend::OpenGL, &CreateOpenGLRHI, "OpenGL");
    }
    void ShutdownModule() override {}
};
} // namespace we::rhi

IMPLEMENT_MODULE(we::rhi::OpenGLRHIModule, WindEffects_OpenGLRHI)

#else

class OpenGLRHIModule : public we::core::IModuleInterface {
public:
    void StartupModule() override {}
    void ShutdownModule() override {}
};

IMPLEMENT_MODULE(OpenGLRHIModule, WindEffects_OpenGLRHI)

#endif
