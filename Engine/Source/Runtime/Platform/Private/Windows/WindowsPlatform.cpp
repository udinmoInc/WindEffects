#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS

#include "Windows/WindowsPlatform.h"
#include "Windows/WindowsPlatformInternal.h"
#include "Windows/Win32String.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LogCategory.h"

#include <commdlg.h>
#include <dwmapi.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>
#include <xinput.h>

#include <algorithm>
#include <cstring>

namespace we::platform {

WindowsPlatform::WindowsPlatform() = default;

WindowsPlatform::~WindowsPlatform() {
    WindowsPlatform::Shutdown();
}

void WindowsPlatform::EnableDpiAwareness() {
    using SetDpiAwarenessContextFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
    if (HMODULE user32 = ::GetModuleHandleW(L"user32.dll")) {
        if (auto fn = reinterpret_cast<SetDpiAwarenessContextFn>(
                ::GetProcAddress(user32, "SetProcessDpiAwarenessContext"))) {
            if (fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                WE_LOG_INFO(we::LogCategory::Startup, "Windows DPI awareness: Per-monitor v2");
                return;
            }
        }
    }
    SetProcessDPIAware();
    WE_LOG_INFO(we::LogCategory::Startup, "Windows DPI awareness: System-aware fallback");
}

void WindowsPlatform::RegisterWindowClass() {
    if (m_ClassRegistered) {
        return;
    }
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = &WindowsPlatform::WndProc;
    wc.hInstance = m_Instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;
    m_ClassAtom = RegisterClassExW(&wc);
    m_ClassRegistered = m_ClassAtom != 0;
    if (!m_ClassRegistered) {
        WE_LOG_ERROR(we::LogCategory::Startup, "Failed to register Win32 window class.");
    }
}

void WindowsPlatform::UnregisterWindowClass() {
    if (m_ClassRegistered) {
        UnregisterClassW(kWindowClassName, m_Instance);
        m_ClassRegistered = false;
        m_ClassAtom = 0;
    }
}

void WindowsPlatform::PopulateCapabilities() {
    m_Capabilities.multipleWindows = true;
    m_Capabilities.highDpi = m_Desc.highDpiAware;
    m_Capabilities.perMonitorDpi = m_Desc.highDpiAware;
    m_Capabilities.borderlessWindow = true;
    m_Capabilities.fullscreen = true;
    m_Capabilities.rawMouse = m_Desc.enableRawInput;
    m_Capabilities.relativeMouse = true;
    m_Capabilities.highResolutionMouse = m_Desc.enableRawInput;
    m_Capabilities.gamepads = m_Desc.enableGamepad;
    m_Capabilities.touch = m_Desc.enableTouch;
    m_Capabilities.clipboard = true;
    m_Capabilities.fileDialogs = true;
    m_Capabilities.messageBoxes = true;
    m_Capabilities.dragAndDrop = true;
    m_Capabilities.directoryWatching = HasService(m_EnabledServices, PlatformService::DirectoryWatch);
    m_Capabilities.darkMode = true;
    m_Capabilities.systemTray = false;
    m_Capabilities.multipleMonitors = true;
    m_Capabilities.threading = true;
    m_Capabilities.dynamicLibraries = true;
    m_Capabilities.highResolutionTimer = true;
    m_Capabilities.console = true;
    m_Capabilities.crashDumps = true;
}

bool WindowsPlatform::Initialize(const PlatformDesc& desc) {
    if (m_Initialized) {
        return true;
    }
    m_Instance = GetModuleHandleW(nullptr);
    QueryPerformanceFrequency(&m_QpcFrequency);
    QueryPerformanceCounter(&m_QpcStart);

    if (desc.highDpiAware) {
        EnableDpiAwareness();
    }

    // COM is required for modern file/folder dialogs and shell APIs.
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    if (!PlatformBackendBase::Initialize(desc)) {
        return false;
    }

    PopulateCapabilities();
    RegisterWindowClass();
    m_CurrentCursor = LoadCursorW(nullptr, IDC_ARROW);
    m_Keys.fill(false);
    m_MouseButtons.fill(false);
    if (!m_ClassRegistered) {
        MakeError(PlatformErrorCode::OsFailure, "Failed to register Win32 window class.", "Initialize",
            static_cast<int32_t>(::GetLastError()));
        return false;
    }
    ClearLastError();
    return true;
}

void WindowsPlatform::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    std::vector<WindowId> windows = GetWindows();
    for (WindowId id : windows) {
        (void)DestroyWindow(id);
    }

    for (auto it = m_Watchers.begin(); it != m_Watchers.end();) {
        (void)UnwatchDirectory(it->second.id);
        it = m_Watchers.begin();
    }

    UnregisterWindowClass();
    PlatformBackendBase::Shutdown();
    CoUninitialize();
}

void WindowsPlatform::RegisterRawInput(HWND hwnd) {
    if (!m_Desc.enableRawInput) {
        return;
    }
    RAWINPUTDEVICE rid{};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02; // mouse
    rid.dwFlags = 0;
    rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

LRESULT CALLBACK WindowsPlatform::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<WindowsPlatform*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }

    if (auto* self = reinterpret_cast<WindowsPlatform*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
        return self->HandleMessage(hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

WindowsPlatform::WindowState* WindowsPlatform::FindWindow(WindowId id) {
    auto it = m_Windows.find(static_cast<uint32_t>(id));
    return it != m_Windows.end() ? &it->second : nullptr;
}

const WindowsPlatform::WindowState* WindowsPlatform::FindWindow(WindowId id) const {
    auto it = m_Windows.find(static_cast<uint32_t>(id));
    return it != m_Windows.end() ? &it->second : nullptr;
}

WindowsPlatform::WindowState* WindowsPlatform::FindWindowByHwnd(HWND hwnd) {
    for (auto& [_, state] : m_Windows) {
        if (state.hwnd == hwnd) {
            return &state;
        }
    }
    return nullptr;
}

KeyModifier WindowsPlatform::QueryModifiers() const {
    KeyModifier mods = KeyModifier::None;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods = mods | KeyModifier::Shift;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods = mods | KeyModifier::Control;
    if (GetKeyState(VK_MENU) & 0x8000) mods = mods | KeyModifier::Alt;
    if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) mods = mods | KeyModifier::Super;
    if (GetKeyState(VK_CAPITAL) & 1) mods = mods | KeyModifier::CapsLock;
    if (GetKeyState(VK_NUMLOCK) & 1) mods = mods | KeyModifier::NumLock;
    return mods;
}

void WindowsPlatform::TrackKey(KeyCode key, bool down) {
    const size_t index = static_cast<size_t>(key);
    if (index < m_Keys.size()) {
        m_Keys[index] = down;
    }
}

void WindowsPlatform::TrackMouseButton(MouseButton button, bool down) {
    const size_t index = static_cast<size_t>(button);
    if (index < m_MouseButtons.size()) {
        m_MouseButtons[index] = down;
    }
}

KeyCode WindowsPlatform::TranslateVirtKey(WPARAM vkey, LPARAM lParam) const {
    const bool extended = (lParam & (1 << 24)) != 0;
    switch (vkey) {
    case 'A': return KeyCode::A; case 'B': return KeyCode::B; case 'C': return KeyCode::C;
    case 'D': return KeyCode::D; case 'E': return KeyCode::E; case 'F': return KeyCode::F;
    case 'G': return KeyCode::G; case 'H': return KeyCode::H; case 'I': return KeyCode::I;
    case 'J': return KeyCode::J; case 'K': return KeyCode::K; case 'L': return KeyCode::L;
    case 'M': return KeyCode::M; case 'N': return KeyCode::N; case 'O': return KeyCode::O;
    case 'P': return KeyCode::P; case 'Q': return KeyCode::Q; case 'R': return KeyCode::R;
    case 'S': return KeyCode::S; case 'T': return KeyCode::T; case 'U': return KeyCode::U;
    case 'V': return KeyCode::V; case 'W': return KeyCode::W; case 'X': return KeyCode::X;
    case 'Y': return KeyCode::Y; case 'Z': return KeyCode::Z;
    case '0': return KeyCode::Num0; case '1': return KeyCode::Num1; case '2': return KeyCode::Num2;
    case '3': return KeyCode::Num3; case '4': return KeyCode::Num4; case '5': return KeyCode::Num5;
    case '6': return KeyCode::Num6; case '7': return KeyCode::Num7; case '8': return KeyCode::Num8;
    case '9': return KeyCode::Num9;
    case VK_ESCAPE: return KeyCode::Escape;
    case VK_TAB: return KeyCode::Tab;
    case VK_CAPITAL: return KeyCode::CapsLock;
    case VK_SPACE: return KeyCode::Space;
    case VK_RETURN: return extended ? KeyCode::NumpadEnter : KeyCode::Enter;
    case VK_BACK: return KeyCode::Backspace;
    case VK_INSERT: return KeyCode::Insert;
    case VK_DELETE: return KeyCode::Delete;
    case VK_HOME: return KeyCode::Home;
    case VK_END: return KeyCode::End;
    case VK_PRIOR: return KeyCode::PageUp;
    case VK_NEXT: return KeyCode::PageDown;
    case VK_LEFT: return KeyCode::Left;
    case VK_RIGHT: return KeyCode::Right;
    case VK_UP: return KeyCode::Up;
    case VK_DOWN: return KeyCode::Down;
    case VK_LSHIFT: return KeyCode::LeftShift;
    case VK_RSHIFT: return KeyCode::RightShift;
    case VK_SHIFT: return KeyCode::LeftShift;
    case VK_LCONTROL: return KeyCode::LeftControl;
    case VK_RCONTROL: return KeyCode::RightControl;
    case VK_CONTROL: return extended ? KeyCode::RightControl : KeyCode::LeftControl;
    case VK_LMENU: return KeyCode::LeftAlt;
    case VK_RMENU: return KeyCode::RightAlt;
    case VK_MENU: return extended ? KeyCode::RightAlt : KeyCode::LeftAlt;
    case VK_LWIN: return KeyCode::LeftSuper;
    case VK_RWIN: return KeyCode::RightSuper;
    case VK_APPS: return KeyCode::Menu;
    case VK_SNAPSHOT: return KeyCode::PrintScreen;
    case VK_SCROLL: return KeyCode::ScrollLock;
    case VK_PAUSE: return KeyCode::Pause;
    case VK_NUMLOCK: return KeyCode::NumLock;
    case VK_F1: return KeyCode::F1; case VK_F2: return KeyCode::F2; case VK_F3: return KeyCode::F3;
    case VK_F4: return KeyCode::F4; case VK_F5: return KeyCode::F5; case VK_F6: return KeyCode::F6;
    case VK_F7: return KeyCode::F7; case VK_F8: return KeyCode::F8; case VK_F9: return KeyCode::F9;
    case VK_F10: return KeyCode::F10; case VK_F11: return KeyCode::F11; case VK_F12: return KeyCode::F12;
    case VK_NUMPAD0: return KeyCode::Numpad0; case VK_NUMPAD1: return KeyCode::Numpad1;
    case VK_NUMPAD2: return KeyCode::Numpad2; case VK_NUMPAD3: return KeyCode::Numpad3;
    case VK_NUMPAD4: return KeyCode::Numpad4; case VK_NUMPAD5: return KeyCode::Numpad5;
    case VK_NUMPAD6: return KeyCode::Numpad6; case VK_NUMPAD7: return KeyCode::Numpad7;
    case VK_NUMPAD8: return KeyCode::Numpad8; case VK_NUMPAD9: return KeyCode::Numpad9;
    case VK_DECIMAL: return KeyCode::NumpadDecimal;
    case VK_DIVIDE: return KeyCode::NumpadDivide;
    case VK_MULTIPLY: return KeyCode::NumpadMultiply;
    case VK_SUBTRACT: return KeyCode::NumpadSubtract;
    case VK_ADD: return KeyCode::NumpadAdd;
    case VK_OEM_1: return KeyCode::Semicolon;
    case VK_OEM_PLUS: return KeyCode::Equal;
    case VK_OEM_COMMA: return KeyCode::Comma;
    case VK_OEM_MINUS: return KeyCode::Minus;
    case VK_OEM_PERIOD: return KeyCode::Period;
    case VK_OEM_2: return KeyCode::Slash;
    case VK_OEM_3: return KeyCode::Grave;
    case VK_OEM_4: return KeyCode::LeftBracket;
    case VK_OEM_5: return KeyCode::Backslash;
    case VK_OEM_6: return KeyCode::RightBracket;
    case VK_OEM_7: return KeyCode::Apostrophe;
    default: return KeyCode::Unknown;
    }
}

} // namespace we::platform
