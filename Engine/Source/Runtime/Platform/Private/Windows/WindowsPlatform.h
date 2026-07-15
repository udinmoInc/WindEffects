#pragma once

#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS

#include "Common/PlatformBackendBase.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <windowsx.h>

// Win32 A/W macros collide with IPlatform method names.
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef LoadLibrary
#undef LoadLibrary
#endif
#ifdef GetMonitorInfo
#undef GetMonitorInfo
#endif
#ifdef GetEnvironmentVariable
#undef GetEnvironmentVariable
#endif
#ifdef SetEnvironmentVariable
#undef SetEnvironmentVariable
#endif
#ifdef GetComputerName
#undef GetComputerName
#endif
#ifdef GetUserName
#undef GetUserName
#endif
#ifdef MessageBox
#undef MessageBox
#endif

#include <array>
#include <atomic>
#include <unordered_map>
#include <vector>

namespace we::platform {

class WindowsPlatform final : public PlatformBackendBase {
public:
    WindowsPlatform();
    ~WindowsPlatform() override;

    [[nodiscard]] const char* GetName() const override { return "Windows"; }

    bool Initialize(const PlatformDesc& desc = {}) override;
    void Shutdown() override;

    bool PollEvents() override;
    void WaitEvents(double timeoutSeconds = -1.0) override;

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

    void SetThreadName(std::string_view name) override;
    void SetThreadPriority(ThreadId id, ThreadPriority priority) override;
    void SetThreadAffinity(ThreadId id, uint64_t mask) override;

    [[nodiscard]] std::string GetExecutablePath() const override;
    [[nodiscard]] std::string GetEnvironmentVariable(std::string_view name) const override;
    bool SetEnvironmentVariable(std::string_view name, std::string_view value) override;
    [[nodiscard]] uint32_t GetProcessId() const override;
    [[nodiscard]] std::vector<std::string> GetCommandLineArgs() const override;
    [[nodiscard]] Result<ProcessLaunchResult> LaunchProcess(const ProcessLaunchDesc& desc) override;
    Result<void> AddLibrarySearchPath(std::string_view directory) override;

    [[nodiscard]] Result<DynamicLibraryId> LoadLibrary(std::string_view path) override;
    Result<void> UnloadLibrary(DynamicLibraryId id) override;
    [[nodiscard]] void* GetLibrarySymbol(DynamicLibraryId id, std::string_view name) const override;

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
    bool AttachConsole() override;
    void ConsoleWrite(std::string_view text) override;

private:
    struct WindowState {
        HWND hwnd = nullptr;
        WindowId id = WindowId::Invalid;
        float dpiScale = 1.0f;
        uint32_t dpi = 96;
        bool relativeMouse = false;
        bool borderless = false;
        bool fullscreen = false;
        WINDOWPLACEMENT savedPlacement{};
        DWORD style = 0;
        DWORD exStyle = 0;
        WindowHitTestFn hitTest = nullptr;
        void* hitTestUserData = nullptr;
        HICON ownedIconBig = nullptr;
        HICON ownedIconSmall = nullptr;
    };

    struct DirectoryWatcher {
        DirectoryWatcherId id = DirectoryWatcherId::Invalid;
        HANDLE directory = INVALID_HANDLE_VALUE;
        HANDLE completion = nullptr;
        std::wstring path;
        bool recursive = true;
        std::byte buffer[64 * 1024]{};
        OVERLAPPED overlapped{};
    };

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void EnableDpiAwareness();
    void RegisterWindowClass();
    void UnregisterWindowClass();
    void RegisterRawInput(HWND hwnd);
    void PollGamepads();
    void PollDirectoryWatchers();
    void PopulateCapabilities();

    [[nodiscard]] WindowState* FindWindow(WindowId id);
    [[nodiscard]] const WindowState* FindWindow(WindowId id) const;
    [[nodiscard]] WindowState* FindWindowByHwnd(HWND hwnd);
    [[nodiscard]] KeyCode TranslateVirtKey(WPARAM vkey, LPARAM lParam) const;
    [[nodiscard]] KeyModifier QueryModifiers() const;
    void TrackKey(KeyCode key, bool down);
    void TrackMouseButton(MouseButton button, bool down);

    HINSTANCE m_Instance = nullptr;
    ATOM m_ClassAtom = 0;
    bool m_ClassRegistered = false;
    bool m_CursorVisible = true;
    HCURSOR m_CurrentCursor = nullptr;

    LARGE_INTEGER m_QpcFrequency{};
    LARGE_INTEGER m_QpcStart{};

    std::unordered_map<uint32_t, WindowState> m_Windows;
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_Keys{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_MouseButtons{};

    static constexpr uint32_t kMaxGamepads = 4;
    struct GamepadState {
        bool connected = false;
        std::array<bool, static_cast<size_t>(GamepadButton::Count)> buttons{};
        std::array<float, static_cast<size_t>(GamepadAxis::Count)> axes{};
        std::string name;
    };
    std::array<GamepadState, kMaxGamepads> m_Gamepads{};

    std::unordered_map<uint32_t, DirectoryWatcher> m_Watchers;
};

} // namespace we::platform

#endif // WE_PLATFORM_WINDOWS
