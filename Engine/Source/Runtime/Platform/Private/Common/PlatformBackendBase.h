#pragma once

#include "Common/EventQueue.h"
#include "Platform/Capabilities.h"
#include "Platform/Diagnostics.h"
#include "Platform/IPlatform.h"
#include "Platform/ThreadSafety.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::platform {

// Shared default implementations for platforms still under development.
// Host backends (e.g. WindowsPlatform) override with native APIs.
class PlatformBackendBase : public IPlatform {
public:
    bool Initialize(const PlatformDesc& desc = {}) override;
    void Shutdown() override;
    [[nodiscard]] bool IsInitialized() const override { return m_Initialized; }
    [[nodiscard]] const char* GetName() const override { return "Unknown"; }

    void ShutdownService(PlatformService service) override;
    [[nodiscard]] bool IsServiceEnabled(PlatformService service) const override;

    [[nodiscard]] const PlatformCapabilities& GetCapabilities() const override { return m_Capabilities; }
    [[nodiscard]] const PlatformDiagnostics& GetDiagnostics() const override { return m_Diagnostics; }
    void ResetDiagnostics() override { m_Diagnostics.Reset(); }

    [[nodiscard]] PlatformError GetLastError() const override { return m_LastError; }
    void ClearLastError() override { m_LastError = PlatformError::Success(); }

    bool PollEvents() override;
    void WaitEvents(double timeoutSeconds = -1.0) override;
    void PostQuit() override;
    [[nodiscard]] bool WantsQuit() const override { return m_WantsQuit; }

    void PushEvent(PlatformEvent event) override;
    void AddEventHandler(EventHandler handler) override;
    [[nodiscard]] std::span<const PlatformEvent> GetFrameEvents() const override;

    Result<WindowId> CreateWindow(const WindowDesc& desc = {}) override;
    Result<void> DestroyWindow(WindowId id) override;
    [[nodiscard]] bool IsWindowValid(WindowId id) const override;
    [[nodiscard]] NativeWindowHandle GetNativeWindowHandle(WindowId id) const override;

    void SetWindowTitle(WindowId id, std::string_view title) override;
    void SetWindowSize(WindowId id, int32_t width, int32_t height) override;
    void SetWindowPosition(WindowId id, int32_t x, int32_t y) override;
    void ShowWindow(WindowId id) override;
    void HideWindow(WindowId id) override;
    void FocusWindow(WindowId id) override;
    void MaximizeWindow(WindowId id) override;
    void MinimizeWindow(WindowId id) override;
    void RestoreWindow(WindowId id) override;
    void SetWindowBorderless(WindowId id, bool borderless) override;
    void SetWindowFullscreen(WindowId id, bool fullscreen, MonitorId monitor = MonitorId::Invalid) override;
    Result<void> ApplyWindowChrome(WindowId id, const WindowChromeDesc& desc = {}) override;
    Result<void> SetWindowIcon(WindowId id, int32_t resourceId, const char* filePath = nullptr) override;

    [[nodiscard]] Uint2 GetWindowSize(WindowId id) const override;
    [[nodiscard]] Uint2 GetWindowPixelSize(WindowId id) const override;
    [[nodiscard]] Int2 GetWindowPosition(WindowId id) const override;
    [[nodiscard]] float GetWindowDpiScale(WindowId id) const override;
    [[nodiscard]] bool IsWindowFocused(WindowId id) const override;
    [[nodiscard]] bool IsWindowMaximized(WindowId id) const override;
    [[nodiscard]] bool IsWindowMinimized(WindowId id) const override;
    [[nodiscard]] std::vector<WindowId> GetWindows() const override;
    void SetWindowHitTest(WindowId id, WindowHitTestFn fn, void* userData = nullptr) override;

    [[nodiscard]] bool IsKeyDown(KeyCode key) const override;
    [[nodiscard]] bool IsMouseButtonDown(MouseButton button) const override;
    [[nodiscard]] Int2 GetMousePosition() const override;
    [[nodiscard]] Int2 GetMousePosition(WindowId id) const override;
    void SetMousePosition(int32_t x, int32_t y) override;
    void SetMousePosition(WindowId id, int32_t x, int32_t y) override;
    void SetRelativeMouseMode(WindowId id, bool enabled) override;
    [[nodiscard]] bool IsRelativeMouseMode(WindowId id) const override;
    void SetCursorVisible(bool visible) override;
    void SetSystemCursor(SystemCursor cursor) override;
    [[nodiscard]] KeyModifier GetKeyModifiers() const override;

    [[nodiscard]] uint32_t GetGamepadCount() const override;
    [[nodiscard]] bool IsGamepadConnected(uint32_t index) const override;
    [[nodiscard]] bool IsGamepadButtonDown(uint32_t index, GamepadButton button) const override;
    [[nodiscard]] float GetGamepadAxis(uint32_t index, GamepadAxis axis) const override;
    void SetGamepadVibration(uint32_t index, float leftMotor, float rightMotor) override;

    Result<void> SetClipboardText(std::string_view text) override;
    [[nodiscard]] std::string GetClipboardText() const override;

    [[nodiscard]] MessageBoxResult ShowMessageBox(const MessageBoxDesc& desc) override;
    [[nodiscard]] std::vector<std::string> ShowFileDialog(const FileDialogDesc& desc) override;

    [[nodiscard]] std::vector<MonitorInfo> GetMonitors() const override;
    [[nodiscard]] MonitorId GetPrimaryMonitor() const override;
    [[nodiscard]] MonitorInfo GetMonitorInfo(MonitorId id) const override;

    [[nodiscard]] double GetTimeSeconds() const override;
    [[nodiscard]] uint64_t GetTimeNanoseconds() const override;
    [[nodiscard]] uint64_t GetHighResolutionCounter() const override;
    [[nodiscard]] uint64_t GetHighResolutionFrequency() const override;
    [[nodiscard]] Result<TimerId> CreateTimer(double intervalSeconds, bool repeating = true) override;
    Result<void> DestroyTimer(TimerId id) override;

    [[nodiscard]] Result<ThreadId> CreateThread(ThreadFn fn, void* userData, std::string_view name = {}) override;
    Result<void> JoinThread(ThreadId id) override;
    Result<void> DetachThread(ThreadId id) override;
    void SetThreadName(std::string_view name) override;
    void SetThreadPriority(ThreadId id, ThreadPriority priority) override;
    void SetThreadAffinity(ThreadId id, uint64_t mask) override;
    [[nodiscard]] ThreadId GetCurrentThreadId() const override;
    [[nodiscard]] ThreadId GetMainThreadId() const override;
    void SleepMilliseconds(uint32_t ms) override;
    void YieldThread() override;

    [[nodiscard]] SyncMutex* CreateMutex() override;
    void DestroyMutex(SyncMutex* m) override;
    void LockMutex(SyncMutex* m) override;
    void UnlockMutex(SyncMutex* m) override;
    [[nodiscard]] SyncEvent* CreateEvent(bool manualReset = false) override;
    void DestroyEvent(SyncEvent* e) override;
    void SignalEvent(SyncEvent* e) override;
    void ResetEvent(SyncEvent* e) override;
    bool WaitEvent(SyncEvent* e, uint32_t timeoutMs = 0xFFFFFFFFu) override;

    [[nodiscard]] std::string GetExecutablePath() const override;
    [[nodiscard]] std::string GetExecutableDirectory() const override;
    [[nodiscard]] std::string GetCurrentWorkingDirectory() const override;
    bool SetCurrentWorkingDirectory(std::string_view path) override;
    [[nodiscard]] std::string GetEnvironmentVariable(std::string_view name) const override;
    bool SetEnvironmentVariable(std::string_view name, std::string_view value) override;
    [[nodiscard]] uint32_t GetProcessId() const override;
    [[nodiscard]] std::vector<std::string> GetCommandLineArgs() const override;
    [[nodiscard]] Result<ProcessLaunchResult> LaunchProcess(const ProcessLaunchDesc& desc) override;
    Result<void> AddLibrarySearchPath(std::string_view directory) override;

    [[nodiscard]] Result<DynamicLibraryId> LoadLibrary(std::string_view path) override;
    Result<void> UnloadLibrary(DynamicLibraryId id) override;
    [[nodiscard]] void* GetLibrarySymbol(DynamicLibraryId id, std::string_view name) const override;

    [[nodiscard]] bool FileExists(std::string_view path) const override;
    [[nodiscard]] bool DirectoryExists(std::string_view path) const override;
    bool CreateDirectories(std::string_view path) override;
    bool DeleteFile(std::string_view path) override;
    bool DeleteDirectory(std::string_view path) override;
    [[nodiscard]] std::optional<std::vector<uint8_t>> ReadFileBytes(std::string_view path) const override;
    bool WriteFileBytes(std::string_view path, std::span<const uint8_t> data) override;
    [[nodiscard]] std::vector<std::string> ListDirectory(std::string_view path) const override;
    [[nodiscard]] std::string GetAbsolutePath(std::string_view path) const override;
    [[nodiscard]] std::string JoinPath(std::string_view a, std::string_view b) const override;
    [[nodiscard]] std::string GetFileName(std::string_view path) const override;
    [[nodiscard]] std::string GetParentPath(std::string_view path) const override;
    [[nodiscard]] std::string GetExtension(std::string_view path) const override;

    [[nodiscard]] Result<DirectoryWatcherId> WatchDirectory(std::string_view path, bool recursive = true) override;
    Result<void> UnwatchDirectory(DirectoryWatcherId id) override;

    [[nodiscard]] MemoryInfo GetMemoryInfo() const override;
    [[nodiscard]] CpuInfo GetCpuInfo() const override;
    [[nodiscard]] std::vector<GpuAdapterInfo> GetGpuAdapters() const override;
    [[nodiscard]] std::string GetLocale() const override;
    [[nodiscard]] std::string GetTimeZone() const override;
    [[nodiscard]] std::string GetComputerName() const override;
    [[nodiscard]] std::string GetUserName() const override;

    void DebugOutput(std::string_view message) override;
    void DebugBreak() override;
    void SetCrashHandler(std::function<void(std::string_view reason)> handler) override;
    void RequestCrashDump(std::string_view reason) override;
    bool AttachConsole() override;
    void ConsoleWrite(std::string_view text) override;

protected:
    void DispatchFrameEvents();
    [[nodiscard]] WindowId AllocateWindowId();
    void SetError(PlatformError error);
    PlatformError MakeError(
        PlatformErrorCode code,
        std::string_view message,
        const char* api,
        int32_t osCode = 0);
    [[nodiscard]] bool RequireService(PlatformService service, const char* api);
    void BeginTimedOp();
    [[nodiscard]] double EndTimedOpMs() const;

    PlatformDesc m_Desc{};
    PlatformCapabilities m_Capabilities{};
    PlatformDiagnostics m_Diagnostics{};
    PlatformError m_LastError{};
    PlatformService m_EnabledServices = PlatformService::All;
    ThreadId m_MainThreadId = ThreadId::Invalid;
    bool m_Initialized = false;
    std::atomic<bool> m_WantsQuit{false};
    EventQueue m_Events;
    std::vector<EventHandler> m_Handlers;
    std::chrono::steady_clock::time_point m_StartTime{};
    std::chrono::steady_clock::time_point m_OpStart{};
    uint32_t m_NextWindowId = 1;
    uint32_t m_NextTimerId = 1;
    uint32_t m_NextLibraryId = 1;
    uint32_t m_NextWatcherId = 1;

    std::function<void(std::string_view)> m_CrashHandler;

    struct ThreadRecord {
        std::thread thread;
        std::string name;
    };
    std::unordered_map<uint64_t, ThreadRecord> m_Threads;
    mutable std::mutex m_ThreadMutex;

    std::unordered_map<uint32_t, void*> m_Libraries;
};

} // namespace we::platform

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
