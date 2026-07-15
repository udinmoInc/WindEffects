#include "RHI/RHIFactory.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <mutex>

#if defined(_WIN32)
#define WE_RHI_STRICMP _stricmp
#else
#include <strings.h>
#define WE_RHI_STRICMP strcasecmp
#endif

namespace we::rhi {
namespace {

struct BackendSlot {
    RHIBackendCreateFn create = nullptr;
    const char* name = nullptr;
};

std::mutex g_Mutex;
std::array<BackendSlot, 8> g_Slots{};

[[nodiscard]] size_t ToIndex(RHIBackend backend) noexcept {
    return static_cast<size_t>(backend);
}

[[nodiscard]] RHIBackend ResolveAutoBackend(bool headless) {
    if (headless) {
        return RHIBackend::Null;
    }
    if (const char* env = std::getenv("WE_RHI_BACKEND")) {
        if (env[0] != '\0') {
            if (WE_RHI_STRICMP(env, "null") == 0) {
                return RHIBackend::Null;
            }
            if (WE_RHI_STRICMP(env, "vulkan") == 0) {
                return RHIBackend::Vulkan;
            }
            if (WE_RHI_STRICMP(env, "dx12") == 0 || WE_RHI_STRICMP(env, "directx12") == 0) {
                return RHIBackend::DirectX12;
            }
            if (WE_RHI_STRICMP(env, "dx11") == 0 || WE_RHI_STRICMP(env, "directx11") == 0) {
                return RHIBackend::DirectX11;
            }
            if (WE_RHI_STRICMP(env, "metal") == 0) {
                return RHIBackend::Metal;
            }
            if (WE_RHI_STRICMP(env, "opengl") == 0) {
                return RHIBackend::OpenGL;
            }
            if (WE_RHI_STRICMP(env, "opengles") == 0) {
                return RHIBackend::OpenGLES;
            }
        }
    }
#if defined(_WIN32) || defined(__linux__)
    return RHIBackend::Vulkan;
#elif defined(__APPLE__)
    return RHIBackend::Metal;
#else
    return RHIBackend::Null;
#endif
}

} // namespace

void RHIFactory::Register(RHIBackend backend, RHIBackendCreateFn createFn, const char* name) {
    if (backend == RHIBackend::Auto || !createFn) {
        return;
    }
    std::scoped_lock lock(g_Mutex);
    auto& slot = g_Slots[ToIndex(backend)];
    slot.create = createFn;
    slot.name = name ? name : ToString(backend);
    WE_LOG_INFO(we::LogCategory::Startup, std::string("RHI backend registered: ") + slot.name);
}

std::unique_ptr<IRHI> RHIFactory::Create(RHIBackend preferred) {
    std::scoped_lock lock(g_Mutex);
    RHIBackend chosen = preferred;
    if (chosen == RHIBackend::Auto) {
        chosen = ResolveAutoBackend(false);
    }

    auto tryCreate = [&](RHIBackend backend) -> std::unique_ptr<IRHI> {
        const auto& slot = g_Slots[ToIndex(backend)];
        if (!slot.create) {
            return nullptr;
        }
        return slot.create();
    };

    if (auto rhi = tryCreate(chosen)) {
        return rhi;
    }

    // Fallbacks: Vulkan → Null
    if (chosen != RHIBackend::Vulkan) {
        if (auto rhi = tryCreate(RHIBackend::Vulkan)) {
            WE_LOG_WARN(we::LogCategory::Startup, "Preferred RHI backend unavailable; falling back to Vulkan.");
            return rhi;
        }
    }
    if (auto rhi = tryCreate(RHIBackend::Null)) {
        WE_LOG_WARN(we::LogCategory::Startup, "Preferred RHI backend unavailable; falling back to NullRHI.");
        return rhi;
    }
    return nullptr;
}

const char* RHIFactory::GetRegisteredName(RHIBackend backend) {
    std::scoped_lock lock(g_Mutex);
    return g_Slots[ToIndex(backend)].name;
}

bool RHIFactory::IsRegistered(RHIBackend backend) {
    std::scoped_lock lock(g_Mutex);
    return g_Slots[ToIndex(backend)].create != nullptr;
}

} // namespace we::rhi
