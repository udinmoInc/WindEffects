#pragma once

#include "Platform/Export.h"
#include "Platform/Types.h"

#include <cstdint>

namespace we::platform {

// Thread-safety contract for public Platform APIs:
//
// MAIN-THREAD ONLY (must call AssertMainThread):
//   Initialize / Shutdown
//   CreateWindow / DestroyWindow / all window mutators
//   PollEvents / WaitEvents / PushEvent (from non-platform producers on main thread)
//   ShowMessageBox / ShowFileDialog
//   SetClipboardText / SetSystemCursor / SetRelativeMouseMode
//   SetWindowHitTest / ApplyWindowChrome / SetWindowIcon
//
// THREAD-SAFE:
//   GetTimeSeconds / GetTimeNanoseconds / GetHighResolutionCounter / Frequency
//   GetCapabilities / GetDiagnostics (snapshot may race; values are diagnostics-only)
//   GetEnvironmentVariable (best-effort)
//   FileExists / DirectoryExists / path utilities / ReadFileBytes / WriteFileBytes
//   SleepMilliseconds / YieldThread / GetCurrentThreadId / IsMainThread
//   CreateMutex / LockMutex / UnlockMutex / CreateEvent / Signal / Wait
//   DebugOutput / ConsoleWrite
//
// AFFINITY REQUIRED (same thread that created the object unless documented):
//   JoinThread / DetachThread / SetThreadPriority / SetThreadAffinity
//   Directory watcher create/destroy (prefer main thread)

[[nodiscard]] PLATFORM_API bool IsMainThread() noexcept;
PLATFORM_API void AssertMainThread(const char* apiName);
PLATFORM_API void AssertNotMainThread(const char* apiName); // rare; reserved for worker helpers

// Internal — called by platform backends during Initialize / Shutdown.
PLATFORM_API void Detail_RegisterMainThread(uint64_t threadKey) noexcept;
PLATFORM_API void Detail_ClearMainThread() noexcept;

} // namespace we::platform
