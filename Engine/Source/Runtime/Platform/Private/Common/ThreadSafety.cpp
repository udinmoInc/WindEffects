#include "Platform/ThreadSafety.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

namespace we::platform {
namespace {

std::atomic<uint64_t> g_MainThreadId{0};
std::atomic<bool> g_MainThreadRegistered{false};

[[nodiscard]] uint64_t CurrentThreadKey() noexcept {
    return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

} // namespace

void Detail_RegisterMainThread(uint64_t threadKey) noexcept {
    g_MainThreadId.store(threadKey, std::memory_order_release);
    g_MainThreadRegistered.store(true, std::memory_order_release);
}

void Detail_ClearMainThread() noexcept {
    g_MainThreadRegistered.store(false, std::memory_order_release);
    g_MainThreadId.store(0, std::memory_order_release);
}

bool IsMainThread() noexcept {
    if (!g_MainThreadRegistered.load(std::memory_order_acquire)) {
        return true;
    }
    return CurrentThreadKey() == g_MainThreadId.load(std::memory_order_acquire);
}

void AssertMainThread(const char* apiName) {
    if (IsMainThread()) {
        return;
    }
    const std::string msg = std::string(apiName ? apiName : "PlatformAPI")
        + " must be called on the main thread.";
    WE_LOG_ERROR(we::LogCategory::Diagnostics, msg);
    WE_CHECK_FATAL(false, "Diagnostics", msg.c_str());
}

void AssertNotMainThread(const char* apiName) {
    if (!IsMainThread()) {
        return;
    }
    const std::string msg = std::string(apiName ? apiName : "PlatformAPI")
        + " must not be called on the main thread.";
    WE_LOG_ERROR(we::LogCategory::Diagnostics, msg);
    WE_CHECK_FATAL(false, "Diagnostics", msg.c_str());
}

} // namespace we::platform
