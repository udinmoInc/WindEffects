#include "RHI/RHI.h"
#include "RHI/RHIFactory.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <cstdlib>
#include <memory>
#include <mutex>

namespace we::rhi {
namespace {

std::unique_ptr<IRHI> g_RHI;
std::once_flag g_InitFlag;
std::mutex g_ShutdownMutex;
RHIInitDesc g_PendingDesc{};
bool g_HasPendingDesc = false;

void EnsureInitialized() {
    std::call_once(g_InitFlag, [] {
        const RHIInitDesc desc = g_HasPendingDesc ? g_PendingDesc : RHIInitDesc{};
        g_RHI = RHIFactory::Create(desc.headless ? RHIBackend::Null : desc.preferredBackend);
        if (!g_RHI) {
            WE_LOG_CRITICAL(we::LogCategory::Startup, "RHIFactory failed to create a backend.");
            return;
        }
        if (!g_RHI->Initialize(desc)) {
            WE_LOG_CRITICAL(we::LogCategory::Startup, "RHI backend Initialize() failed.");
            g_RHI.reset();
        }
    });
}

} // namespace

IRHI& RHI::Initialize(const RHIInitDesc& desc) {
    {
        std::scoped_lock lock(g_ShutdownMutex);
        if (!g_RHI) {
            g_PendingDesc = desc;
            g_HasPendingDesc = true;
        }
    }
    EnsureInitialized();
    if (!g_RHI) {
        WE_LOG_CRITICAL(we::LogCategory::Startup, "RHI::Initialize() failed — backend unavailable.");
        std::abort();
    }
    return *g_RHI;
}

IRHI& RHI::Get() {
    EnsureInitialized();
    if (!g_RHI) {
        WE_LOG_CRITICAL(we::LogCategory::Startup, "RHI::Get() called but backend is unavailable.");
        std::abort();
    }
    return *g_RHI;
}

bool RHI::IsAvailable() noexcept {
    try {
        EnsureInitialized();
    } catch (...) {
        return false;
    }
    return g_RHI != nullptr;
}

void RHI::Shutdown() {
    std::scoped_lock lock(g_ShutdownMutex);
    if (g_RHI) {
        g_RHI->Shutdown();
        g_RHI.reset();
    }
}

} // namespace we::rhi
