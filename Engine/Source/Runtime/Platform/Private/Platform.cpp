#include "Platform/Platform.h"
#include "Platform/PlatformFactory.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <cstdlib>
#include <memory>
#include <mutex>

namespace we::platform {
namespace {

std::unique_ptr<IPlatform> g_Platform;
std::once_flag g_InitFlag;
std::mutex g_ShutdownMutex;
PlatformDesc g_PendingDesc{};
bool g_HasPendingDesc = false;

void EnsureInitialized() {
    std::call_once(g_InitFlag, [] {
        g_Platform.reset(PlatformFactory::Create());
        if (!g_Platform) {
            WE_LOG_CRITICAL(we::LogCategory::Startup, "PlatformFactory failed to create a backend.");
            return;
        }
        const PlatformDesc desc = g_HasPendingDesc ? g_PendingDesc : PlatformDesc{};
        if (!g_Platform->Initialize(desc)) {
            WE_LOG_CRITICAL(we::LogCategory::Startup, "Platform backend Initialize() failed.");
            g_Platform.reset();
        }
    });
}

} // namespace

IPlatform& Platform::Initialize(const PlatformDesc& desc) {
    {
        std::scoped_lock lock(g_ShutdownMutex);
        if (!g_Platform) {
            g_PendingDesc = desc;
            g_HasPendingDesc = true;
        }
    }
    EnsureInitialized();
    if (!g_Platform) {
        WE_LOG_CRITICAL(we::LogCategory::Startup, "Platform::Initialize() failed — backend unavailable.");
        std::abort();
    }
    return *g_Platform;
}

IPlatform& Platform::Get() {
    EnsureInitialized();
    if (!g_Platform) {
        WE_LOG_CRITICAL(we::LogCategory::Startup, "Platform::Get() called but backend is unavailable.");
        std::abort();
    }
    return *g_Platform;
}

bool Platform::IsAvailable() noexcept {
    try {
        EnsureInitialized();
    } catch (...) {
        return false;
    }
    return g_Platform != nullptr;
}

void Platform::Shutdown() {
    std::scoped_lock lock(g_ShutdownMutex);
    if (g_Platform) {
        g_Platform->Shutdown();
        g_Platform.reset();
    }
}

} // namespace we::platform
