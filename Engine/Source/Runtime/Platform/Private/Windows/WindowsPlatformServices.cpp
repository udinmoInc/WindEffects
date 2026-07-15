#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS

#include "Windows/WindowsPlatform.h"
#include "Windows/Win32String.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <commdlg.h>
#include <dxgi.h>
#include <intrin.h>
#include <shellapi.h>
#include <shlobj.h>
#include <xinput.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")

#ifdef LoadLibrary
#undef LoadLibrary
#endif
#ifdef CreateWindow
#undef CreateWindow
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

namespace we::platform {
namespace {

float NormalizeAxis(SHORT value, SHORT deadzone) {
    if (value > -deadzone && value < deadzone) {
        return 0.0f;
    }
    return std::clamp(static_cast<float>(value) / 32767.0f, -1.0f, 1.0f);
}

float NormalizeTrigger(BYTE value) {
    constexpr BYTE deadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    if (value < deadzone) {
        return 0.0f;
    }
    return static_cast<float>(value) / 255.0f;
}

} // namespace

void WindowsPlatform::PollGamepads() {
    if (!m_Desc.enableGamepad) {
        return;
    }

    for (uint32_t i = 0; i < kMaxGamepads; ++i) {
        XINPUT_STATE xstate{};
        const DWORD result = XInputGetState(i, &xstate);
        GamepadState& pad = m_Gamepads[i];
        const bool connected = result == ERROR_SUCCESS;

        if (connected != pad.connected) {
            pad.connected = connected;
            pad.name = connected ? "XInput Controller" : "";
            PushEvent(GamepadConnectionEvent{i, connected, pad.name});
            if (!connected) {
                pad.buttons.fill(false);
                pad.axes.fill(0.0f);
                continue;
            }
        }

        if (!connected) {
            continue;
        }

        const auto setButton = [&](GamepadButton button, bool down) {
            const size_t idx = static_cast<size_t>(button);
            if (pad.buttons[idx] != down) {
                pad.buttons[idx] = down;
                PushEvent(GamepadButtonEvent{i, button, down});
            }
        };

        const WORD b = xstate.Gamepad.wButtons;
        setButton(GamepadButton::A, (b & XINPUT_GAMEPAD_A) != 0);
        setButton(GamepadButton::B, (b & XINPUT_GAMEPAD_B) != 0);
        setButton(GamepadButton::X, (b & XINPUT_GAMEPAD_X) != 0);
        setButton(GamepadButton::Y, (b & XINPUT_GAMEPAD_Y) != 0);
        setButton(GamepadButton::Back, (b & XINPUT_GAMEPAD_BACK) != 0);
        setButton(GamepadButton::Start, (b & XINPUT_GAMEPAD_START) != 0);
        setButton(GamepadButton::LeftStick, (b & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
        setButton(GamepadButton::RightStick, (b & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);
        setButton(GamepadButton::LeftShoulder, (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
        setButton(GamepadButton::RightShoulder, (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
        setButton(GamepadButton::DPadUp, (b & XINPUT_GAMEPAD_DPAD_UP) != 0);
        setButton(GamepadButton::DPadDown, (b & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
        setButton(GamepadButton::DPadLeft, (b & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
        setButton(GamepadButton::DPadRight, (b & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);

        const auto setAxis = [&](GamepadAxis axis, float value) {
            const size_t idx = static_cast<size_t>(axis);
            if (std::fabs(pad.axes[idx] - value) > 0.01f) {
                pad.axes[idx] = value;
                PushEvent(GamepadAxisEvent{i, axis, value});
            }
        };

        setAxis(GamepadAxis::LeftX, NormalizeAxis(xstate.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        setAxis(GamepadAxis::LeftY, NormalizeAxis(xstate.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
        setAxis(GamepadAxis::RightX, NormalizeAxis(xstate.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        setAxis(GamepadAxis::RightY, NormalizeAxis(xstate.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE));
        setAxis(GamepadAxis::LeftTrigger, NormalizeTrigger(xstate.Gamepad.bLeftTrigger));
        setAxis(GamepadAxis::RightTrigger, NormalizeTrigger(xstate.Gamepad.bRightTrigger));
    }
}

uint32_t WindowsPlatform::GetGamepadCount() const {
    uint32_t count = 0;
    for (const auto& pad : m_Gamepads) {
        if (pad.connected) {
            ++count;
        }
    }
    return count;
}

bool WindowsPlatform::IsGamepadConnected(uint32_t index) const {
    return index < kMaxGamepads && m_Gamepads[index].connected;
}

bool WindowsPlatform::IsGamepadButtonDown(uint32_t index, GamepadButton button) const {
    if (index >= kMaxGamepads) {
        return false;
    }
    const size_t idx = static_cast<size_t>(button);
    return idx < m_Gamepads[index].buttons.size() ? m_Gamepads[index].buttons[idx] : false;
}

float WindowsPlatform::GetGamepadAxis(uint32_t index, GamepadAxis axis) const {
    if (index >= kMaxGamepads) {
        return 0.0f;
    }
    const size_t idx = static_cast<size_t>(axis);
    return idx < m_Gamepads[index].axes.size() ? m_Gamepads[index].axes[idx] : 0.0f;
}

void WindowsPlatform::SetGamepadVibration(uint32_t index, float leftMotor, float rightMotor) {
    if (index >= kMaxGamepads) {
        return;
    }
    XINPUT_VIBRATION vibration{};
    vibration.wLeftMotorSpeed = static_cast<WORD>(std::clamp(leftMotor, 0.0f, 1.0f) * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(std::clamp(rightMotor, 0.0f, 1.0f) * 65535.0f);
    XInputSetState(index, &vibration);
}

Result<void> WindowsPlatform::SetClipboardText(std::string_view text) {
    AssertMainThread("SetClipboardText");
    if (!OpenClipboard(nullptr)) {
        return MakeError(PlatformErrorCode::OsFailure, "OpenClipboard failed.", "SetClipboardText",
            static_cast<int32_t>(::GetLastError()));
    }
    EmptyClipboard();
    const std::wstring wide = win32::Utf8ToWide(text);
    const SIZE_T bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!mem) {
        CloseClipboard();
        return MakeError(PlatformErrorCode::OutOfMemory, "GlobalAlloc failed for clipboard.", "SetClipboardText");
    }
    void* locked = GlobalLock(mem);
    if (!locked) {
        GlobalFree(mem);
        CloseClipboard();
        return MakeError(PlatformErrorCode::OsFailure, "GlobalLock failed for clipboard.", "SetClipboardText");
    }
    std::memcpy(locked, wide.c_str(), bytes);
    GlobalUnlock(mem);
    SetClipboardData(CF_UNICODETEXT, mem);
    CloseClipboard();
    ClearLastError();
    return Result<void>::Success();
}

std::string WindowsPlatform::GetClipboardText() const {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(nullptr)) {
        return {};
    }
    std::string result;
    if (HANDLE data = GetClipboardData(CF_UNICODETEXT)) {
        if (const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data))) {
            result = win32::WideToUtf8(text);
            GlobalUnlock(data);
        }
    }
    CloseClipboard();
    return result;
}

MessageBoxResult WindowsPlatform::ShowMessageBox(const MessageBoxDesc& desc) {
    UINT type = MB_OK;
    if (desc.yesNo) {
        type = MB_YESNO;
    } else if (desc.okCancel) {
        type = MB_OKCANCEL;
    }

    switch (desc.type) {
    case MessageBoxType::Warning: type |= MB_ICONWARNING; break;
    case MessageBoxType::Error: type |= MB_ICONERROR; break;
    case MessageBoxType::Question: type |= MB_ICONQUESTION; break;
    default: type |= MB_ICONINFORMATION; break;
    }

    const std::wstring title = win32::Utf8ToWide(desc.title ? desc.title : "WindEffects");
    const std::wstring message = win32::Utf8ToWide(desc.message ? desc.message : "");
    const int result = MessageBoxW(nullptr, message.c_str(), title.c_str(), type);
    switch (result) {
    case IDYES: return MessageBoxResult::Yes;
    case IDNO: return MessageBoxResult::No;
    case IDCANCEL: return MessageBoxResult::Cancel;
    default: return MessageBoxResult::Ok;
    }
}

std::vector<std::string> WindowsPlatform::ShowFileDialog(const FileDialogDesc& desc) {
    std::vector<std::string> results;

    if (desc.mode == FileDialogMode::OpenFolder) {
        IFileDialog* dialog = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
            return results;
        }
        DWORD options = 0;
        dialog->GetOptions(&options);
        dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        if (SUCCEEDED(dialog->Show(nullptr))) {
            IShellItem* item = nullptr;
            if (SUCCEEDED(dialog->GetResult(&item))) {
                PWSTR path = nullptr;
                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                    results.push_back(win32::WideToUtf8(path));
                    CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        dialog->Release();
        return results;
    }

    wchar_t fileBuffer[32768]{};
    if (desc.defaultName && *desc.defaultName) {
        const std::wstring defaultName = win32::Utf8ToWide(desc.defaultName);
        wcsncpy_s(fileBuffer, defaultName.c_str(), _TRUNCATE);
    }

    std::wstring filter;
    if (desc.filters.empty()) {
        filter.append(L"All Files");
        filter.push_back(L'\0');
        filter.append(L"*.*");
        filter.push_back(L'\0');
        filter.push_back(L'\0');
    } else {
        for (const auto& f : desc.filters) {
            filter.append(win32::Utf8ToWide(f.name ? f.name : "Files"));
            filter.push_back(L'\0');
            filter.append(win32::Utf8ToWide(f.pattern ? f.pattern : "*.*"));
            filter.push_back(L'\0');
        }
        filter.push_back(L'\0');
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileBuffer));
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (desc.mode == FileDialogMode::OpenFiles) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
    }
    if (desc.mode == FileDialogMode::SaveFile) {
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    }

    const std::wstring title = win32::Utf8ToWide(desc.title ? desc.title : "Select File");
    ofn.lpstrTitle = title.c_str();
    const std::wstring defaultPath = win32::Utf8ToWide(desc.defaultPath ? desc.defaultPath : "");
    if (!defaultPath.empty()) {
        ofn.lpstrInitialDir = defaultPath.c_str();
    }

    const BOOL ok = (desc.mode == FileDialogMode::SaveFile)
        ? GetSaveFileNameW(&ofn)
        : GetOpenFileNameW(&ofn);

    if (!ok) {
        return results;
    }

    if (desc.mode == FileDialogMode::OpenFiles) {
        const wchar_t* ptr = fileBuffer;
        std::wstring directory = ptr;
        ptr += directory.size() + 1;
        if (*ptr == L'\0') {
            results.push_back(win32::WideToUtf8(directory));
        } else {
            while (*ptr) {
                std::wstring full = directory + L"\\" + ptr;
                results.push_back(win32::WideToUtf8(full));
                ptr += std::wcslen(ptr) + 1;
            }
        }
    } else {
        results.push_back(win32::WideToUtf8(fileBuffer));
    }
    return results;
}

namespace {

BOOL CALLBACK MonitorEnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam) {
    auto* out = reinterpret_cast<std::vector<MonitorInfo>*>(lParam);
    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) {
        return TRUE;
    }

    MonitorInfo info{};
    info.id = static_cast<MonitorId>(static_cast<uint32_t>(out->size()) + 1);
    info.name = win32::WideToUtf8(mi.szDevice);
    info.bounds = {mi.rcMonitor.left, mi.rcMonitor.top,
                   mi.rcMonitor.right - mi.rcMonitor.left,
                   mi.rcMonitor.bottom - mi.rcMonitor.top};
    info.workArea = {mi.rcWork.left, mi.rcWork.top,
                     mi.rcWork.right - mi.rcWork.left,
                     mi.rcWork.bottom - mi.rcWork.top};
    info.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

    UINT dpiX = 96;
    UINT dpiY = 96;
    using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    if (HMODULE shcore = ::LoadLibraryW(L"Shcore.dll")) {
        if (auto fn = reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"))) {
            fn(monitor, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY);
        }
        FreeLibrary(shcore);
    }
    info.dpi = dpiX;
    info.dpiScale = static_cast<float>(dpiX) / 96.0f;

    DEVMODEW mode{};
    mode.dmSize = sizeof(mode);
    if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
        DisplayMode dm{};
        dm.width = mode.dmPelsWidth;
        dm.height = mode.dmPelsHeight;
        dm.refreshRateHz = mode.dmDisplayFrequency;
        dm.bitsPerPixel = mode.dmBitsPerPel;
        info.modes.push_back(dm);
    }

    out->push_back(std::move(info));
    return TRUE;
}

} // namespace

std::vector<MonitorInfo> WindowsPlatform::GetMonitors() const {
    std::vector<MonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

MonitorId WindowsPlatform::GetPrimaryMonitor() const {
    for (const auto& monitor : GetMonitors()) {
        if (monitor.primary) {
            return monitor.id;
        }
    }
    return MonitorId::Invalid;
}

MonitorInfo WindowsPlatform::GetMonitorInfo(MonitorId id) const {
    for (const auto& monitor : GetMonitors()) {
        if (monitor.id == id) {
            return monitor;
        }
    }
    return {};
}

double WindowsPlatform::GetTimeSeconds() const {
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - m_QpcStart.QuadPart) /
           static_cast<double>(m_QpcFrequency.QuadPart);
}

uint64_t WindowsPlatform::GetTimeNanoseconds() const {
    return static_cast<uint64_t>(GetTimeSeconds() * 1'000'000'000.0);
}

uint64_t WindowsPlatform::GetHighResolutionCounter() const {
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    return static_cast<uint64_t>(now.QuadPart);
}

uint64_t WindowsPlatform::GetHighResolutionFrequency() const {
    return static_cast<uint64_t>(m_QpcFrequency.QuadPart);
}

void WindowsPlatform::SetThreadName(std::string_view name) {
    const std::wstring wide = win32::Utf8ToWide(name);
    using SetThreadDescriptionFn = HRESULT(WINAPI*)(HANDLE, PCWSTR);
    if (HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll")) {
        if (auto fn = reinterpret_cast<SetThreadDescriptionFn>(::GetProcAddress(kernel, "SetThreadDescription"))) {
            fn(GetCurrentThread(), wide.c_str());
        }
    }
}

void WindowsPlatform::SetThreadPriority(ThreadId /*id*/, ThreadPriority priority) {
    int winPriority = THREAD_PRIORITY_NORMAL;
    switch (priority) {
    case ThreadPriority::Lowest: winPriority = THREAD_PRIORITY_LOWEST; break;
    case ThreadPriority::BelowNormal: winPriority = THREAD_PRIORITY_BELOW_NORMAL; break;
    case ThreadPriority::AboveNormal: winPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
    case ThreadPriority::Highest: winPriority = THREAD_PRIORITY_HIGHEST; break;
    case ThreadPriority::TimeCritical: winPriority = THREAD_PRIORITY_TIME_CRITICAL; break;
    default: break;
    }
    ::SetThreadPriority(GetCurrentThread(), winPriority);
}

void WindowsPlatform::SetThreadAffinity(ThreadId /*id*/, uint64_t mask) {
    SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(mask));
}

std::string WindowsPlatform::GetExecutablePath() const {
    wchar_t path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return {};
    }
    return win32::WideToUtf8(std::wstring_view(path, len));
}

std::string WindowsPlatform::GetEnvironmentVariable(std::string_view name) const {
    const std::wstring wideName = win32::Utf8ToWide(name);
    const DWORD needed = GetEnvironmentVariableW(wideName.c_str(), nullptr, 0);
    if (needed == 0) {
        return {};
    }
    std::wstring value(needed, L'\0');
    GetEnvironmentVariableW(wideName.c_str(), value.data(), needed);
    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return win32::WideToUtf8(value);
}

bool WindowsPlatform::SetEnvironmentVariable(std::string_view name, std::string_view value) {
    return ::SetEnvironmentVariableW(win32::Utf8ToWide(name).c_str(), win32::Utf8ToWide(value).c_str()) != FALSE;
}

uint32_t WindowsPlatform::GetProcessId() const {
    return static_cast<uint32_t>(::GetCurrentProcessId());
}

std::vector<std::string> WindowsPlatform::GetCommandLineArgs() const {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    if (!argv) {
        return args;
    }
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.push_back(win32::WideToUtf8(argv[i]));
    }
    LocalFree(argv);
    return args;
}

Result<DynamicLibraryId> WindowsPlatform::LoadLibrary(std::string_view path) {
    HMODULE module = ::LoadLibraryW(win32::Utf8ToWide(path).c_str());
    if (!module) {
        return MakeError(PlatformErrorCode::OsFailure, "LoadLibraryW failed.", "LoadLibrary",
            static_cast<int32_t>(::GetLastError()));
    }
    const uint32_t id = m_NextLibraryId++;
    m_Libraries[id] = module;
    ClearLastError();
    return static_cast<DynamicLibraryId>(id);
}

Result<void> WindowsPlatform::UnloadLibrary(DynamicLibraryId id) {
    auto it = m_Libraries.find(static_cast<uint32_t>(id));
    if (it == m_Libraries.end()) {
        return MakeError(PlatformErrorCode::InvalidHandle, "Unknown dynamic library id.", "UnloadLibrary");
    }
    FreeLibrary(static_cast<HMODULE>(it->second));
    m_Libraries.erase(it);
    ClearLastError();
    return Result<void>::Success();
}

void* WindowsPlatform::GetLibrarySymbol(DynamicLibraryId id, std::string_view name) const {
    auto it = m_Libraries.find(static_cast<uint32_t>(id));
    if (it == m_Libraries.end()) {
        return nullptr;
    }
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(it->second), std::string(name).c_str()));
}

Result<DirectoryWatcherId> WindowsPlatform::WatchDirectory(std::string_view path, bool recursive) {
    if (!RequireService(PlatformService::DirectoryWatch, "WatchDirectory")) {
        return m_LastError;
    }
    BeginTimedOp();
    const std::wstring wide = win32::Utf8ToWide(path);
    HANDLE directory = CreateFileW(
        wide.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);
    if (directory == INVALID_HANDLE_VALUE) {
        return MakeError(PlatformErrorCode::OsFailure, "CreateFileW failed for directory watch.", "WatchDirectory",
            static_cast<int32_t>(::GetLastError()));
    }

    DirectoryWatcher watcher{};
    watcher.id = static_cast<DirectoryWatcherId>(m_NextWatcherId++);
    watcher.directory = directory;
    watcher.path = wide;
    watcher.recursive = recursive;
    watcher.overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    const BOOL ok = ReadDirectoryChangesW(
        watcher.directory,
        watcher.buffer,
        static_cast<DWORD>(sizeof(watcher.buffer)),
        recursive ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
        nullptr,
        &watcher.overlapped,
        nullptr);

    if (!ok) {
        const DWORD err = ::GetLastError();
        CloseHandle(watcher.overlapped.hEvent);
        CloseHandle(directory);
        return MakeError(PlatformErrorCode::OsFailure, "ReadDirectoryChangesW failed.", "WatchDirectory",
            static_cast<int32_t>(err));
    }

    const uint32_t key = static_cast<uint32_t>(watcher.id);
    m_Watchers.emplace(key, std::move(watcher));
    ++m_Diagnostics.directoryWatchersCreated;
    m_Diagnostics.lastDirectoryPollMs = EndTimedOpMs();
    ClearLastError();
    return static_cast<DirectoryWatcherId>(key);
}

Result<void> WindowsPlatform::UnwatchDirectory(DirectoryWatcherId id) {
    auto it = m_Watchers.find(static_cast<uint32_t>(id));
    if (it == m_Watchers.end()) {
        return MakeError(PlatformErrorCode::InvalidHandle, "Unknown directory watcher id.", "UnwatchDirectory");
    }
    CancelIo(it->second.directory);
    CloseHandle(it->second.overlapped.hEvent);
    CloseHandle(it->second.directory);
    m_Watchers.erase(it);
    ClearLastError();
    return Result<void>::Success();
}

Result<ProcessLaunchResult> WindowsPlatform::LaunchProcess(const ProcessLaunchDesc& desc) {
    if (!desc.executable || desc.executable[0] == '\0') {
        return MakeError(PlatformErrorCode::InvalidArgument, "Executable path is empty.", "LaunchProcess");
    }

    std::wstring commandLine = L"\"" + win32::Utf8ToWide(desc.executable) + L"\"";
    for (const auto& arg : desc.arguments) {
        commandLine += L" \"";
        commandLine += win32::Utf8ToWide(arg);
        commandLine += L"\"";
    }

    std::wstring workingDir;
    const wchar_t* cwd = nullptr;
    if (desc.workingDirectory && desc.workingDirectory[0] != '\0') {
        workingDir = win32::Utf8ToWide(desc.workingDirectory);
        cwd = workingDir.c_str();
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring mutableCmd = commandLine;
    const BOOL ok = CreateProcessW(
        nullptr,
        mutableCmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        cwd,
        &si,
        &pi);

    if (!ok) {
        return MakeError(PlatformErrorCode::OsFailure, "CreateProcessW failed.", "LaunchProcess",
            static_cast<int32_t>(::GetLastError()));
    }

    ProcessLaunchResult result{};
    result.launched = true;
    result.processId = pi.dwProcessId;
    if (desc.waitForExit) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        result.exitCode = static_cast<int32_t>(exitCode);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    ClearLastError();
    return result;
}

Result<void> WindowsPlatform::AddLibrarySearchPath(std::string_view directory) {
    if (directory.empty()) {
        return MakeError(PlatformErrorCode::InvalidArgument, "Directory is empty.", "AddLibrarySearchPath");
    }
    const std::wstring wide = win32::Utf8ToWide(directory);
    // Prefer AddDllDirectory when available; fall back to SetDllDirectoryW.
    using AddDllDirectoryFn = void*(WINAPI*)(PCWSTR);
    if (HMODULE kernel = ::GetModuleHandleW(L"kernel32.dll")) {
        if (auto fn = reinterpret_cast<AddDllDirectoryFn>(::GetProcAddress(kernel, "AddDllDirectory"))) {
            if (fn(wide.c_str()) != nullptr) {
                ClearLastError();
                return Result<void>::Success();
            }
        }
    }
    if (!SetDllDirectoryW(wide.c_str())) {
        return MakeError(PlatformErrorCode::OsFailure, "SetDllDirectoryW failed.", "AddLibrarySearchPath",
            static_cast<int32_t>(::GetLastError()));
    }
    ClearLastError();
    return Result<void>::Success();
}

void WindowsPlatform::PollDirectoryWatchers() {
    for (auto& [_, watcher] : m_Watchers) {
        DWORD bytes = 0;
        if (!GetOverlappedResult(watcher.directory, &watcher.overlapped, &bytes, FALSE)) {
            continue;
        }
        if (bytes == 0) {
            continue;
        }

        auto* notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(watcher.buffer);
        for (;;) {
            const std::wstring fileName(notify->FileName, notify->FileNameLength / sizeof(wchar_t));
            DirectoryChangeEvent event;
            event.watcher = watcher.id;
            event.path = JoinPath(win32::WideToUtf8(watcher.path), win32::WideToUtf8(fileName));
            switch (notify->Action) {
            case FILE_ACTION_ADDED: event.action = DirectoryWatchAction::Added; break;
            case FILE_ACTION_REMOVED: event.action = DirectoryWatchAction::Removed; break;
            case FILE_ACTION_MODIFIED: event.action = DirectoryWatchAction::Modified; break;
            case FILE_ACTION_RENAMED_OLD_NAME: event.action = DirectoryWatchAction::RenamedOld; break;
            case FILE_ACTION_RENAMED_NEW_NAME: event.action = DirectoryWatchAction::RenamedNew; break;
            default: event.action = DirectoryWatchAction::Modified; break;
            }
            PushEvent(std::move(event));
            ++m_Diagnostics.directoryNotifications;

            if (notify->NextEntryOffset == 0) {
                break;
            }
            notify = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<uint8_t*>(notify) + notify->NextEntryOffset);
        }

        ::ResetEvent(watcher.overlapped.hEvent);
        ReadDirectoryChangesW(
            watcher.directory,
            watcher.buffer,
            static_cast<DWORD>(sizeof(watcher.buffer)),
            watcher.recursive ? TRUE : FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
            nullptr,
            &watcher.overlapped,
            nullptr);
    }
}

MemoryInfo WindowsPlatform::GetMemoryInfo() const {
    MEMORYSTATUSEX status{sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&status);

    struct ProcessMemoryCounters {
        DWORD cb;
        DWORD PageFaultCount;
        SIZE_T PeakWorkingSetSize;
        SIZE_T WorkingSetSize;
        SIZE_T QuotaPeakPagedPoolUsage;
        SIZE_T QuotaPagedPoolUsage;
        SIZE_T QuotaPeakNonPagedPoolUsage;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
    };

    using K32Fn = BOOL(WINAPI*)(HANDLE, ProcessMemoryCounters*, DWORD);
    SIZE_T workingSet = 0;
    if (HMODULE k = ::GetModuleHandleW(L"kernel32.dll")) {
        if (auto fn = reinterpret_cast<K32Fn>(::GetProcAddress(k, "K32GetProcessMemoryInfo"))) {
            ProcessMemoryCounters counters{};
            counters.cb = sizeof(counters);
            if (fn(GetCurrentProcess(), &counters, sizeof(counters))) {
                workingSet = counters.WorkingSetSize;
            }
        }
    }

    MemoryInfo info{};
    info.totalPhysicalBytes = status.ullTotalPhys;
    info.availablePhysicalBytes = status.ullAvailPhys;
    info.totalVirtualBytes = status.ullTotalVirtual;
    info.availableVirtualBytes = status.ullAvailVirtual;
    info.processUsedBytes = static_cast<uint64_t>(workingSet);
    return info;
}

CpuInfo WindowsPlatform::GetCpuInfo() const {
    CpuInfo info = PlatformBackendBase::GetCpuInfo();

    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    info.logicalCores = sysInfo.dwNumberOfProcessors;
    info.pageSize = sysInfo.dwPageSize;

    int cpuInfo[4]{};
#if defined(_MSC_VER)
    __cpuid(cpuInfo, 0x80000000);
    const unsigned maxExt = static_cast<unsigned>(cpuInfo[0]);
    char brand[0x40]{};
    if (maxExt >= 0x80000004) {
        __cpuid(reinterpret_cast<int*>(brand + 0), 0x80000002);
        __cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
        __cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
        info.brand = brand;
    }
#endif

    DWORD length = 0;
    GetLogicalProcessorInformation(nullptr, &length);
    if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER && length > 0) {
        std::vector<uint8_t> buffer(length);
        auto* ptr = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(buffer.data());
        if (GetLogicalProcessorInformation(ptr, &length)) {
            uint32_t cores = 0;
            const size_t count = length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            for (size_t i = 0; i < count; ++i) {
                if (ptr[i].Relationship == RelationProcessorCore) {
                    ++cores;
                }
            }
            info.physicalCores = cores;
            info.hyperThreading = info.logicalCores > info.physicalCores;
        }
    }
    return info;
}

std::vector<GpuAdapterInfo> WindowsPlatform::GetGpuAdapters() const {
    std::vector<GpuAdapterInfo> adapters;
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) || !factory) {
        return adapters;
    }

    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        adapter->GetDesc1(&desc);
        GpuAdapterInfo info{};
        info.index = i;
        info.name = win32::WideToUtf8(desc.Description);
        info.dedicatedVideoMemory = desc.DedicatedVideoMemory;
        info.dedicatedSystemMemory = desc.DedicatedSystemMemory;
        info.sharedSystemMemory = desc.SharedSystemMemory;
        info.vendorId = desc.VendorId;
        info.deviceId = desc.DeviceId;
        info.isSoftware = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        adapters.push_back(std::move(info));
        adapter->Release();
        adapter = nullptr;
    }
    factory->Release();
    return adapters;
}

std::string WindowsPlatform::GetLocale() const {
    wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
    if (GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) > 0) {
        return win32::WideToUtf8(locale);
    }
    return "en-US";
}

std::string WindowsPlatform::GetTimeZone() const {
    DYNAMIC_TIME_ZONE_INFORMATION tz{};
    GetDynamicTimeZoneInformation(&tz);
    return win32::WideToUtf8(tz.TimeZoneKeyName);
}

std::string WindowsPlatform::GetComputerName() const {
    wchar_t name[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    if (GetComputerNameW(name, &size)) {
        return win32::WideToUtf8(std::wstring_view(name, size));
    }
    return {};
}

std::string WindowsPlatform::GetUserName() const {
    wchar_t name[256]{};
    DWORD size = 256;
    if (GetUserNameW(name, &size)) {
        return win32::WideToUtf8(std::wstring_view(name, size > 0 ? size - 1 : 0));
    }
    return {};
}

void WindowsPlatform::DebugOutput(std::string_view message) {
    const std::wstring wide = win32::Utf8ToWide(message);
    OutputDebugStringW(wide.c_str());
    OutputDebugStringW(L"\n");
    PlatformBackendBase::DebugOutput(message);
}

bool WindowsPlatform::AttachConsole() {
    if (::AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        FILE* unused = nullptr;
        freopen_s(&unused, "CONOUT$", "w", stdout);
        freopen_s(&unused, "CONOUT$", "w", stderr);
        freopen_s(&unused, "CONIN$", "r", stdin);
        return true;
    }
    return false;
}

void WindowsPlatform::ConsoleWrite(std::string_view text) {
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out == INVALID_HANDLE_VALUE || out == nullptr) {
        PlatformBackendBase::ConsoleWrite(text);
        return;
    }
    const std::wstring wide = win32::Utf8ToWide(text);
    DWORD written = 0;
    WriteConsoleW(out, wide.c_str(), static_cast<DWORD>(wide.size()), &written, nullptr);
}

} // namespace we::platform

#endif // WE_PLATFORM_WINDOWS
