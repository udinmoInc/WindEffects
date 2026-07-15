#pragma once

#include "Platform/Capabilities.h"
#include "Platform/Diagnostics.h"
#include "Platform/Events.h"
#include "Platform/Export.h"
#include "Platform/InputTypes.h"
#include "Platform/NativeHandle.h"
#include "Platform/Result.h"
#include "Platform/Types.h"
#include "Platform/UndefWin32Macros.h"

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::platform {

using EventHandler = std::function<void(const PlatformEvent&)>;

// Abstract platform backend. Engine code must go through Platform::Get(), never OS APIs.
//
// Design principles:
// - Opaque handles only in the public API (no HWND / Display* / ANativeWindow*).
// - Capability queries instead of platform #ifdefs in Engine / Editor / Tools.
// - Failures report via Result<T> and/or GetLastError() — no silent failures.
// - Main-thread vs thread-safe contract documented in ThreadSafety.h.
//
// If a TU includes windows.h after this header, also include Platform/UndefWin32Macros.h
// afterward so CreateWindow / LoadLibrary / etc. stay usable as method names.
class PLATFORM_API IPlatform {
public:
    virtual ~IPlatform() = default;

    // ---- Lifecycle ----------------------------------------------------------
    // MAIN-THREAD. Services in PlatformDesc control which subsystems start.
    virtual bool Initialize(const PlatformDesc& desc = {}) = 0;
    virtual void Shutdown() = 0;
    [[nodiscard]] virtual bool IsInitialized() const = 0;
    [[nodiscard]] virtual const char* GetName() const = 0;

    // Independent service shut-down (default tears down only that group).
    virtual void ShutdownService(PlatformService service) = 0;
    [[nodiscard]] virtual bool IsServiceEnabled(PlatformService service) const = 0;

    // ---- Capabilities / diagnostics / errors --------------------------------
    [[nodiscard]] virtual const PlatformCapabilities& GetCapabilities() const = 0;
    [[nodiscard]] virtual const PlatformDiagnostics& GetDiagnostics() const = 0;
    virtual void ResetDiagnostics() = 0;

    [[nodiscard]] virtual PlatformError GetLastError() const = 0;
    virtual void ClearLastError() = 0;

    // ---- Events -------------------------------------------------------------
    // MAIN-THREAD. Returns false when a QuitEvent has been requested.
    virtual bool PollEvents() = 0;
    virtual void WaitEvents(double timeoutSeconds = -1.0) = 0;
    virtual void PostQuit() = 0;
    [[nodiscard]] virtual bool WantsQuit() const = 0;

    virtual void PushEvent(PlatformEvent event) = 0;
    virtual void AddEventHandler(EventHandler handler) = 0;
    [[nodiscard]] virtual std::span<const PlatformEvent> GetFrameEvents() const = 0;

    // ---- Windows ------------------------------------------------------------
    // MAIN-THREAD.
    [[nodiscard]] virtual Result<WindowId> CreateWindow(const WindowDesc& desc = {}) = 0;
    virtual Result<void> DestroyWindow(WindowId id) = 0;
    [[nodiscard]] virtual bool IsWindowValid(WindowId id) const = 0;
    [[nodiscard]] virtual NativeWindowHandle GetNativeWindowHandle(WindowId id) const = 0;

    virtual void SetWindowTitle(WindowId id, std::string_view title) = 0;
    virtual void SetWindowSize(WindowId id, int32_t width, int32_t height) = 0;
    virtual void SetWindowPosition(WindowId id, int32_t x, int32_t y) = 0;
    virtual void ShowWindow(WindowId id) = 0;
    virtual void HideWindow(WindowId id) = 0;
    virtual void FocusWindow(WindowId id) = 0;
    virtual void MaximizeWindow(WindowId id) = 0;
    virtual void MinimizeWindow(WindowId id) = 0;
    virtual void RestoreWindow(WindowId id) = 0;
    virtual void SetWindowBorderless(WindowId id, bool borderless) = 0;
    virtual void SetWindowFullscreen(WindowId id, bool fullscreen, MonitorId monitor = MonitorId::Invalid) = 0;

    // Apply OS chrome (corners / border color). No-op when unsupported.
    virtual Result<void> ApplyWindowChrome(WindowId id, const WindowChromeDesc& desc = {}) = 0;
    // Sets window / taskbar icons. resourceId is a host-module icon resource (Windows RT_GROUP_ICON).
    // Other backends may ignore resourceId and use filePath when provided.
    virtual Result<void> SetWindowIcon(WindowId id, int32_t resourceId, const char* filePath = nullptr) = 0;

    [[nodiscard]] virtual Uint2 GetWindowSize(WindowId id) const = 0;
    [[nodiscard]] virtual Uint2 GetWindowPixelSize(WindowId id) const = 0;
    [[nodiscard]] virtual Int2 GetWindowPosition(WindowId id) const = 0;
    [[nodiscard]] virtual float GetWindowDpiScale(WindowId id) const = 0;
    [[nodiscard]] virtual bool IsWindowFocused(WindowId id) const = 0;
    [[nodiscard]] virtual bool IsWindowMaximized(WindowId id) const = 0;
    [[nodiscard]] virtual bool IsWindowMinimized(WindowId id) const = 0;
    [[nodiscard]] virtual std::vector<WindowId> GetWindows() const = 0;

    // Custom non-client hit testing for borderless chrome (drag/resize zones).
    virtual void SetWindowHitTest(WindowId id, WindowHitTestFn fn, void* userData = nullptr) = 0;

    // ---- Keyboard / Mouse ---------------------------------------------------
    [[nodiscard]] virtual bool IsKeyDown(KeyCode key) const = 0;
    [[nodiscard]] virtual bool IsMouseButtonDown(MouseButton button) const = 0;
    [[nodiscard]] virtual Int2 GetMousePosition() const = 0;
    [[nodiscard]] virtual Int2 GetMousePosition(WindowId id) const = 0;
    virtual void SetMousePosition(int32_t x, int32_t y) = 0;
    virtual void SetMousePosition(WindowId id, int32_t x, int32_t y) = 0;
    virtual void SetRelativeMouseMode(WindowId id, bool enabled) = 0;
    [[nodiscard]] virtual bool IsRelativeMouseMode(WindowId id) const = 0;
    virtual void SetCursorVisible(bool visible) = 0;
    virtual void SetSystemCursor(SystemCursor cursor) = 0;
    [[nodiscard]] virtual KeyModifier GetKeyModifiers() const = 0;

    // ---- Gamepad ------------------------------------------------------------
    [[nodiscard]] virtual uint32_t GetGamepadCount() const = 0;
    [[nodiscard]] virtual bool IsGamepadConnected(uint32_t index) const = 0;
    [[nodiscard]] virtual bool IsGamepadButtonDown(uint32_t index, GamepadButton button) const = 0;
    [[nodiscard]] virtual float GetGamepadAxis(uint32_t index, GamepadAxis axis) const = 0;
    virtual void SetGamepadVibration(uint32_t index, float leftMotor, float rightMotor) = 0;

    // ---- Clipboard / Drag-drop ----------------------------------------------
    virtual Result<void> SetClipboardText(std::string_view text) = 0;
    [[nodiscard]] virtual std::string GetClipboardText() const = 0;

    // ---- Dialogs ------------------------------------------------------------
    // MAIN-THREAD.
    [[nodiscard]] virtual MessageBoxResult ShowMessageBox(const MessageBoxDesc& desc) = 0;
    [[nodiscard]] virtual std::vector<std::string> ShowFileDialog(const FileDialogDesc& desc) = 0;

    // ---- Monitors / Display -------------------------------------------------
    [[nodiscard]] virtual std::vector<MonitorInfo> GetMonitors() const = 0;
    [[nodiscard]] virtual MonitorId GetPrimaryMonitor() const = 0;
    [[nodiscard]] virtual MonitorInfo GetMonitorInfo(MonitorId id) const = 0;

    // ---- Time ---------------------------------------------------------------
    // THREAD-SAFE.
    [[nodiscard]] virtual double GetTimeSeconds() const = 0;
    [[nodiscard]] virtual uint64_t GetTimeNanoseconds() const = 0;
    [[nodiscard]] virtual uint64_t GetHighResolutionCounter() const = 0;
    [[nodiscard]] virtual uint64_t GetHighResolutionFrequency() const = 0;
    [[nodiscard]] virtual Result<TimerId> CreateTimer(double intervalSeconds, bool repeating = true) = 0;
    virtual Result<void> DestroyTimer(TimerId id) = 0;

    // ---- Threading ----------------------------------------------------------
    using ThreadFn = void (*)(void* userData);
    [[nodiscard]] virtual Result<ThreadId> CreateThread(ThreadFn fn, void* userData, std::string_view name = {}) = 0;
    virtual Result<void> JoinThread(ThreadId id) = 0;
    virtual Result<void> DetachThread(ThreadId id) = 0;
    virtual void SetThreadName(std::string_view name) = 0;
    virtual void SetThreadPriority(ThreadId id, ThreadPriority priority) = 0;
    virtual void SetThreadAffinity(ThreadId id, uint64_t mask) = 0;
    [[nodiscard]] virtual ThreadId GetCurrentThreadId() const = 0;
    [[nodiscard]] virtual ThreadId GetMainThreadId() const = 0;
    virtual void SleepMilliseconds(uint32_t ms) = 0;
    virtual void YieldThread() = 0;

    // Mutex / event primitives (lightweight handles wrapping OS objects)
    [[nodiscard]] virtual SyncMutex* CreateMutex() = 0;
    virtual void DestroyMutex(SyncMutex* m) = 0;
    virtual void LockMutex(SyncMutex* m) = 0;
    virtual void UnlockMutex(SyncMutex* m) = 0;
    [[nodiscard]] virtual SyncEvent* CreateEvent(bool manualReset = false) = 0;
    virtual void DestroyEvent(SyncEvent* e) = 0;
    virtual void SignalEvent(SyncEvent* e) = 0;
    virtual void ResetEvent(SyncEvent* e) = 0;
    virtual bool WaitEvent(SyncEvent* e, uint32_t timeoutMs = 0xFFFFFFFFu) = 0;

    // ---- Process / Environment ----------------------------------------------
    [[nodiscard]] virtual std::string GetExecutablePath() const = 0;
    [[nodiscard]] virtual std::string GetExecutableDirectory() const = 0;
    [[nodiscard]] virtual std::string GetCurrentWorkingDirectory() const = 0;
    virtual bool SetCurrentWorkingDirectory(std::string_view path) = 0;
    [[nodiscard]] virtual std::string GetEnvironmentVariable(std::string_view name) const = 0;
    virtual bool SetEnvironmentVariable(std::string_view name, std::string_view value) = 0;
    [[nodiscard]] virtual uint32_t GetProcessId() const = 0;
    [[nodiscard]] virtual std::vector<std::string> GetCommandLineArgs() const = 0;
    [[nodiscard]] virtual Result<ProcessLaunchResult> LaunchProcess(const ProcessLaunchDesc& desc) = 0;
    virtual Result<void> AddLibrarySearchPath(std::string_view directory) = 0;

    // ---- Dynamic libraries --------------------------------------------------
    [[nodiscard]] virtual Result<DynamicLibraryId> LoadLibrary(std::string_view path) = 0;
    virtual Result<void> UnloadLibrary(DynamicLibraryId id) = 0;
    [[nodiscard]] virtual void* GetLibrarySymbol(DynamicLibraryId id, std::string_view name) const = 0;

    // ---- File system --------------------------------------------------------
    // THREAD-SAFE (path utilities / existence / byte IO).
    [[nodiscard]] virtual bool FileExists(std::string_view path) const = 0;
    [[nodiscard]] virtual bool DirectoryExists(std::string_view path) const = 0;
    virtual bool CreateDirectories(std::string_view path) = 0;
    virtual bool DeleteFile(std::string_view path) = 0;
    virtual bool DeleteDirectory(std::string_view path) = 0;
    [[nodiscard]] virtual std::optional<std::vector<uint8_t>> ReadFileBytes(std::string_view path) const = 0;
    virtual bool WriteFileBytes(std::string_view path, std::span<const uint8_t> data) = 0;
    [[nodiscard]] virtual std::vector<std::string> ListDirectory(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string GetAbsolutePath(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string JoinPath(std::string_view a, std::string_view b) const = 0;
    [[nodiscard]] virtual std::string GetFileName(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string GetParentPath(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string GetExtension(std::string_view path) const = 0;

    [[nodiscard]] virtual Result<DirectoryWatcherId> WatchDirectory(std::string_view path, bool recursive = true) = 0;
    virtual Result<void> UnwatchDirectory(DirectoryWatcherId id) = 0;

    // ---- System info --------------------------------------------------------
    [[nodiscard]] virtual MemoryInfo GetMemoryInfo() const = 0;
    [[nodiscard]] virtual CpuInfo GetCpuInfo() const = 0;
    [[nodiscard]] virtual std::vector<GpuAdapterInfo> GetGpuAdapters() const = 0;
    [[nodiscard]] virtual std::string GetLocale() const = 0;
    [[nodiscard]] virtual std::string GetTimeZone() const = 0;
    [[nodiscard]] virtual std::string GetComputerName() const = 0;
    [[nodiscard]] virtual std::string GetUserName() const = 0;

    // ---- Debug / crash / console --------------------------------------------
    virtual void DebugOutput(std::string_view message) = 0;
    virtual void DebugBreak() = 0;
    virtual void SetCrashHandler(std::function<void(std::string_view reason)> handler) = 0;
    virtual void RequestCrashDump(std::string_view reason) = 0;
    virtual bool AttachConsole() = 0;
    virtual void ConsoleWrite(std::string_view text) = 0;
};

} // namespace we::platform
