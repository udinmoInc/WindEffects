#include "RHI/GpuBackends.h"

#include <mutex>

namespace we::rhi {
namespace {

std::mutex g_Mutex;
GpuSceneBackendFactory g_SceneFactory = nullptr;
GpuUIBackendFactory g_UIFactory = nullptr;

} // namespace

void GpuBackendRegistry::RegisterScene(GpuSceneBackendFactory factory) {
    std::scoped_lock lock(g_Mutex);
    g_SceneFactory = factory;
}

void GpuBackendRegistry::RegisterUI(GpuUIBackendFactory factory) {
    std::scoped_lock lock(g_Mutex);
    g_UIFactory = factory;
}

std::unique_ptr<IGpuSceneBackend> GpuBackendRegistry::CreateScene() {
    std::scoped_lock lock(g_Mutex);
    return g_SceneFactory ? g_SceneFactory() : nullptr;
}

std::unique_ptr<IGpuUIBackend> GpuBackendRegistry::CreateUI() {
    std::scoped_lock lock(g_Mutex);
    return g_UIFactory ? g_UIFactory() : nullptr;
}

bool GpuBackendRegistry::HasScene() noexcept {
    std::scoped_lock lock(g_Mutex);
    return g_SceneFactory != nullptr;
}

bool GpuBackendRegistry::HasUI() noexcept {
    std::scoped_lock lock(g_Mutex);
    return g_UIFactory != nullptr;
}

} // namespace we::rhi
