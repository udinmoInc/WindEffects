#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_WINDOWS

#include "Windows/WindowsPlatform.h"
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
namespace {

constexpr const wchar_t* kWindowClassName = L"WindEffects.Platform.Window";

void CenterOnMonitor(HWND hwnd, int32_t width, int32_t height, MonitorId /*monitor*/) {
    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi{sizeof(MONITORINFO)};
    if (!GetMonitorInfoW(monitor, &mi)) {
        return;
    }
    const int x = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - width) / 2;
    const int y = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - height) / 2;
    SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

} // namespace

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

LRESULT WindowsPlatform::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WindowState* window = FindWindowByHwnd(hwnd);

    switch (msg) {
    case WM_CLOSE:
        if (window) {
            PushEvent(WindowCloseEvent{window->id});
        }
        return 0;

    case WM_DESTROY:
        return 0;

    case WM_SETFOCUS:
        if (window) {
            PushEvent(WindowFocusEvent{window->id, true});
        }
        return 0;

    case WM_KILLFOCUS:
        if (window) {
            PushEvent(WindowFocusEvent{window->id, false});
        }
        m_Keys.fill(false);
        return 0;

    case WM_MOVE:
        if (window) {
            PushEvent(WindowMoveEvent{window->id, {static_cast<int32_t>(LOWORD(lParam)), static_cast<int32_t>(HIWORD(lParam))}});
        }
        return 0;

    case WM_SIZE: {
        if (!window) {
            return 0;
        }
        const UINT width = LOWORD(lParam);
        const UINT height = HIWORD(lParam);
        RECT client{};
        GetClientRect(hwnd, &client);
        const Uint2 logical{static_cast<uint32_t>(client.right - client.left),
                            static_cast<uint32_t>(client.bottom - client.top)};
        PushEvent(WindowResizeEvent{window->id, logical, logical});

        if (wParam == SIZE_MINIMIZED) {
            PushEvent(WindowMinimizeEvent{window->id, true});
        } else if (wParam == SIZE_MAXIMIZED) {
            PushEvent(WindowMaximizeEvent{window->id, true});
        } else if (wParam == SIZE_RESTORED) {
            PushEvent(WindowMinimizeEvent{window->id, false});
            PushEvent(WindowMaximizeEvent{window->id, false});
        }
        (void)width;
        (void)height;
        return 0;
    }

    case WM_DPICHANGED: {
        if (!window) {
            return 0;
        }
        window->dpi = HIWORD(wParam);
        window->dpiScale = static_cast<float>(window->dpi) / 96.0f;
        const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
        SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
        PushEvent(WindowDpiEvent{window->id, window->dpiScale, window->dpi});
        return 0;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        if (!window) {
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }
        const bool pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        const bool repeat = pressed && (lParam & (1 << 30)) != 0;
        const KeyCode key = TranslateVirtKey(wParam, lParam);
        TrackKey(key, pressed);
        PushEvent(KeyEvent{
            window->id,
            key,
            QueryModifiers(),
            pressed,
            repeat,
            static_cast<uint32_t>((lParam >> 16) & 0xFF)
        });
        return 0;
    }

    case WM_CHAR:
        if (window && wParam >= 32) {
            PushEvent(TextInputEvent{window->id, static_cast<char32_t>(wParam)});
        }
        return 0;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
        if (!window) {
            return 0;
        }
        MouseButton button = MouseButton::Left;
        bool pressed = false;
        switch (msg) {
        case WM_LBUTTONDOWN: button = MouseButton::Left; pressed = true; break;
        case WM_LBUTTONUP: button = MouseButton::Left; pressed = false; break;
        case WM_RBUTTONDOWN: button = MouseButton::Right; pressed = true; break;
        case WM_RBUTTONUP: button = MouseButton::Right; pressed = false; break;
        case WM_MBUTTONDOWN: button = MouseButton::Middle; pressed = true; break;
        case WM_MBUTTONUP: button = MouseButton::Middle; pressed = false; break;
        case WM_XBUTTONDOWN:
            button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
            pressed = true;
            break;
        case WM_XBUTTONUP:
            button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
            pressed = false;
            break;
        default: break;
        }
        TrackMouseButton(button, pressed);
        if (pressed) {
            SetCapture(hwnd);
        } else if (!m_MouseButtons[0] && !m_MouseButtons[1] && !m_MouseButtons[2]) {
            ReleaseCapture();
        }
        PushEvent(MouseButtonEvent{
            window->id,
            button,
            QueryModifiers(),
            pressed,
            {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            1
        });
        return 0;
    }

    case WM_MOUSEMOVE:
        if (window) {
            PushEvent(MouseMoveEvent{
                window->id,
                {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
                {},
                false
            });
        }
        return 0;

    case WM_MOUSEWHEEL:
        if (window) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);
            PushEvent(MouseWheelEvent{window->id, {0.0f, delta}, {pt.x, pt.y}});
        }
        return 0;

    case WM_MOUSEHWHEEL:
        if (window) {
            const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / static_cast<float>(WHEEL_DELTA);
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);
            PushEvent(MouseWheelEvent{window->id, {delta, 0.0f}, {pt.x, pt.y}});
        }
        return 0;

    case WM_INPUT: {
        if (!window || !window->relativeMouse) {
            break;
        }
        UINT size = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size == 0) {
            break;
        }
        std::vector<uint8_t> buffer(size);
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1)) {
            break;
        }
        const auto* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEMOUSE) {
            const float dx = static_cast<float>(raw->data.mouse.lLastX);
            const float dy = static_cast<float>(raw->data.mouse.lLastY);
            PushEvent(RawMouseEvent{window->id, {dx, dy}, static_cast<int32_t>(raw->data.mouse.ulButtons)});
            PushEvent(MouseMoveEvent{window->id, {}, {dx, dy}, true});
        }
        break;
    }

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursor(m_CursorVisible ? m_CurrentCursor : nullptr);
            return TRUE;
        }
        break;

    case WM_DROPFILES:
        if (window) {
            auto drop = reinterpret_cast<HDROP>(wParam);
            const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
            DropFilesEvent event;
            event.window = window->id;
            POINT pt{};
            DragQueryPoint(drop, &pt);
            event.position = {pt.x, pt.y};
            for (UINT i = 0; i < count; ++i) {
                const UINT len = DragQueryFileW(drop, i, nullptr, 0);
                std::wstring path(len, L'\0');
                DragQueryFileW(drop, i, path.data(), len + 1);
                event.paths.push_back(win32::WideToUtf8(path));
            }
            DragFinish(drop);
            PushEvent(std::move(event));
        }
        return 0;

    case WM_NCHITTEST:
        if (window && window->hitTest) {
            POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);
            const WindowHitTestResult result = window->hitTest(window->id, {pt.x, pt.y}, window->hitTestUserData);
            switch (result) {
            case WindowHitTestResult::Draggable: return HTCAPTION;
            case WindowHitTestResult::ResizeTopLeft: return HTTOPLEFT;
            case WindowHitTestResult::ResizeTop: return HTTOP;
            case WindowHitTestResult::ResizeTopRight: return HTTOPRIGHT;
            case WindowHitTestResult::ResizeRight: return HTRIGHT;
            case WindowHitTestResult::ResizeBottomRight: return HTBOTTOMRIGHT;
            case WindowHitTestResult::ResizeBottom: return HTBOTTOM;
            case WindowHitTestResult::ResizeBottomLeft: return HTBOTTOMLEFT;
            case WindowHitTestResult::ResizeLeft: return HTLEFT;
            default: return HTCLIENT;
            }
        }
        break;

    default:
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool WindowsPlatform::PollEvents() {
    MSG msg{};
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            PostQuit();
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    PollGamepads();
    PollDirectoryWatchers();
    return PlatformBackendBase::PollEvents();
}

void WindowsPlatform::WaitEvents(double timeoutSeconds) {
    const UINT timeoutMs = timeoutSeconds < 0.0
        ? INFINITE
        : static_cast<UINT>(timeoutSeconds * 1000.0);
    MsgWaitForMultipleObjects(0, nullptr, FALSE, timeoutMs, QS_ALLINPUT);
    PollEvents();
}

Result<WindowId> WindowsPlatform::CreateWindow(const WindowDesc& desc) {
    AssertMainThread("CreateWindow");
    if (!RequireService(PlatformService::Windowing, "CreateWindow")) {
        return m_LastError;
    }
    if (!m_ClassRegistered) {
        return MakeError(PlatformErrorCode::NotInitialized, "Window class is not registered.", "CreateWindow");
    }

    BeginTimedOp();

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!desc.resizable) {
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }
    if (desc.borderless) {
        style = WS_POPUP;
    }

    DWORD exStyle = WS_EX_APPWINDOW;
    RECT rect{0, 0, desc.width, desc.height};
    AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    const int x = desc.x < 0 ? CW_USEDEFAULT : desc.x;
    const int y = desc.y < 0 ? CW_USEDEFAULT : desc.y;

    const WindowId id = AllocateWindowId();
    const std::wstring title = win32::Utf8ToWide(desc.title ? desc.title : "WindEffects");

    HWND hwnd = CreateWindowExW(
        exStyle,
        kWindowClassName,
        title.c_str(),
        style,
        x,
        y,
        width,
        height,
        nullptr,
        nullptr,
        m_Instance,
        this);

    if (!hwnd) {
        const DWORD err = ::GetLastError();
        WE_LOG_ERROR(we::LogCategory::Startup, "CreateWindowExW failed.");
        return MakeError(PlatformErrorCode::OsFailure, "CreateWindowExW failed.", "CreateWindow",
            static_cast<int32_t>(err));
    }

    WindowState state{};
    state.hwnd = hwnd;
    state.id = id;
    state.borderless = desc.borderless;
    state.style = style;
    state.exStyle = exStyle;
    state.savedPlacement.length = sizeof(WINDOWPLACEMENT);

    using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
    if (HMODULE user32 = ::GetModuleHandleW(L"user32.dll")) {
        if (auto fn = reinterpret_cast<GetDpiForWindowFn>(::GetProcAddress(user32, "GetDpiForWindow"))) {
            state.dpi = fn(hwnd);
            state.dpiScale = static_cast<float>(state.dpi) / 96.0f;
        }
    }

    m_Windows.emplace(static_cast<uint32_t>(id), state);
    RegisterRawInput(hwnd);

    if (desc.acceptDropFiles) {
        DragAcceptFiles(hwnd, TRUE);
    }

    if (desc.x < 0 && desc.y < 0) {
        CenterOnMonitor(hwnd, width, height, desc.monitor);
    }

    if (desc.maximized) {
        ::ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    } else if (desc.minimized) {
        ::ShowWindow(hwnd, SW_SHOWMINIMIZED);
    } else if (desc.visible) {
        ::ShowWindow(hwnd, SW_SHOW);
    } else {
        ::ShowWindow(hwnd, SW_HIDE);
    }

    if (desc.fullscreen) {
        SetWindowFullscreen(id, true, desc.monitor);
    }

    UpdateWindow(hwnd);

    RECT client{};
    GetClientRect(hwnd, &client);
    PushEvent(WindowResizeEvent{
        id,
        {static_cast<uint32_t>(client.right - client.left), static_cast<uint32_t>(client.bottom - client.top)},
        {static_cast<uint32_t>(client.right - client.left), static_cast<uint32_t>(client.bottom - client.top)}
    });

    m_Diagnostics.lastWindowCreateMs = EndTimedOpMs();
    m_Diagnostics.totalWindowCreateMs += m_Diagnostics.lastWindowCreateMs;
    ++m_Diagnostics.windowsCreated;
    ClearLastError();
    return id;
}

Result<void> WindowsPlatform::DestroyWindow(WindowId id) {
    AssertMainThread("DestroyWindow");
    if (WindowState* state = FindWindow(id)) {
        if (state->ownedIconBig) {
            DestroyIcon(state->ownedIconBig);
            state->ownedIconBig = nullptr;
        }
        if (state->ownedIconSmall) {
            DestroyIcon(state->ownedIconSmall);
            state->ownedIconSmall = nullptr;
        }
        if (state->hwnd) {
            ::DestroyWindow(state->hwnd);
        }
        m_Windows.erase(static_cast<uint32_t>(id));
        ++m_Diagnostics.windowsDestroyed;
        ClearLastError();
        return Result<void>::Success();
    }
    return MakeError(PlatformErrorCode::InvalidHandle, "Window id is invalid.", "DestroyWindow");
}

Result<void> WindowsPlatform::ApplyWindowChrome(WindowId id, const WindowChromeDesc& desc) {
    AssertMainThread("ApplyWindowChrome");
    WindowState* state = FindWindow(id);
    if (!state || !state->hwnd) {
        return MakeError(PlatformErrorCode::InvalidHandle, "Window id is invalid.", "ApplyWindowChrome");
    }

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif
#ifndef DWMWCP_DEFAULT
#define DWMWCP_DEFAULT 0
#endif

    const int cornerPreference = desc.roundedCorners ? DWMWCP_ROUND : DWMWCP_DEFAULT;
    DwmSetWindowAttribute(state->hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPreference, sizeof(cornerPreference));

    const COLORREF borderColor = RGB(
        (desc.borderColorRgb >> 16) & 0xFF,
        (desc.borderColorRgb >> 8) & 0xFF,
        desc.borderColorRgb & 0xFF);
    DwmSetWindowAttribute(state->hwnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
    ClearLastError();
    return Result<void>::Success();
}

Result<void> WindowsPlatform::SetWindowIcon(WindowId id, int32_t resourceId, const char* filePath) {
    AssertMainThread("SetWindowIcon");
    WindowState* state = FindWindow(id);
    if (!state || !state->hwnd) {
        return MakeError(PlatformErrorCode::InvalidHandle, "Window id is invalid.", "SetWindowIcon");
    }

    HICON big = nullptr;
    HICON smallIcon = nullptr;
    const int bigW = GetSystemMetrics(SM_CXICON);
    const int bigH = GetSystemMetrics(SM_CYICON);
    const int smW = GetSystemMetrics(SM_CXSMICON);
    const int smH = GetSystemMetrics(SM_CYSMICON);

    if (filePath && filePath[0] != '\0') {
        const std::wstring wide = win32::Utf8ToWide(filePath);
        big = static_cast<HICON>(LoadImageW(nullptr, wide.c_str(), IMAGE_ICON, bigW, bigH, LR_LOADFROMFILE));
        smallIcon = static_cast<HICON>(LoadImageW(nullptr, wide.c_str(), IMAGE_ICON, smW, smH, LR_LOADFROMFILE));
    } else if (resourceId > 0) {
        HMODULE module = GetModuleHandleW(nullptr);
        big = static_cast<HICON>(LoadImageW(
            module, MAKEINTRESOURCEW(resourceId), IMAGE_ICON, bigW, bigH, LR_DEFAULTCOLOR));
        smallIcon = static_cast<HICON>(LoadImageW(
            module, MAKEINTRESOURCEW(resourceId), IMAGE_ICON, smW, smH, LR_DEFAULTCOLOR));
        if (!big) {
            big = static_cast<HICON>(LoadImageW(
                module, MAKEINTRESOURCEW(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        }
        if (!smallIcon) {
            smallIcon = static_cast<HICON>(LoadImageW(
                module, MAKEINTRESOURCEW(resourceId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        }
    } else {
        return MakeError(PlatformErrorCode::InvalidArgument, "No icon resource or file path provided.", "SetWindowIcon");
    }

    if (!big && !smallIcon) {
        return MakeError(PlatformErrorCode::NotFound, "Failed to load window icon.", "SetWindowIcon",
            static_cast<int32_t>(::GetLastError()));
    }

    if (state->ownedIconBig) {
        DestroyIcon(state->ownedIconBig);
        state->ownedIconBig = nullptr;
    }
    if (state->ownedIconSmall) {
        DestroyIcon(state->ownedIconSmall);
        state->ownedIconSmall = nullptr;
    }

    if (big) {
        SendMessageW(state->hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big));
        state->ownedIconBig = big;
    }
    if (smallIcon) {
        SendMessageW(state->hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        state->ownedIconSmall = smallIcon;
    }

    ClearLastError();
    return Result<void>::Success();
}

bool WindowsPlatform::IsWindowValid(WindowId id) const {
    return FindWindow(id) != nullptr;
}

NativeWindowHandle WindowsPlatform::GetNativeWindowHandle(WindowId id) const {
    NativeWindowHandle handle{};
    if (const WindowState* state = FindWindow(id)) {
        handle.type = NativeWindowType::Win32Hwnd;
        handle.window = state->hwnd;
        handle.display = m_Instance;
    }
    return handle;
}

void WindowsPlatform::SetWindowTitle(WindowId id, std::string_view title) {
    if (WindowState* state = FindWindow(id)) {
        const std::wstring wide = win32::Utf8ToWide(title);
        SetWindowTextW(state->hwnd, wide.c_str());
    }
}

void WindowsPlatform::SetWindowSize(WindowId id, int32_t width, int32_t height) {
    if (WindowState* state = FindWindow(id)) {
        RECT rect{0, 0, width, height};
        AdjustWindowRectEx(&rect, static_cast<DWORD>(GetWindowLongPtrW(state->hwnd, GWL_STYLE)), FALSE,
            static_cast<DWORD>(GetWindowLongPtrW(state->hwnd, GWL_EXSTYLE)));
        SetWindowPos(state->hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void WindowsPlatform::SetWindowPosition(WindowId id, int32_t x, int32_t y) {
    if (WindowState* state = FindWindow(id)) {
        SetWindowPos(state->hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void WindowsPlatform::ShowWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        ::ShowWindow(state->hwnd, SW_SHOW);
    }
}

void WindowsPlatform::HideWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        ::ShowWindow(state->hwnd, SW_HIDE);
    }
}

void WindowsPlatform::FocusWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        SetForegroundWindow(state->hwnd);
        SetFocus(state->hwnd);
    }
}

void WindowsPlatform::MaximizeWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        ::ShowWindow(state->hwnd, SW_MAXIMIZE);
    }
}

void WindowsPlatform::MinimizeWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        ::ShowWindow(state->hwnd, SW_MINIMIZE);
    }
}

void WindowsPlatform::RestoreWindow(WindowId id) {
    if (WindowState* state = FindWindow(id)) {
        ::ShowWindow(state->hwnd, SW_RESTORE);
    }
}

void WindowsPlatform::SetWindowBorderless(WindowId id, bool borderless) {
    WindowState* state = FindWindow(id);
    if (!state) {
        return;
    }
    state->borderless = borderless;
    const LONG_PTR style = borderless ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    SetWindowLongPtrW(state->hwnd, GWL_STYLE, style);
    SetWindowPos(state->hwnd, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void WindowsPlatform::SetWindowFullscreen(WindowId id, bool fullscreen, MonitorId /*monitor*/) {
    WindowState* state = FindWindow(id);
    if (!state) {
        return;
    }
    if (fullscreen == state->fullscreen) {
        return;
    }
    if (fullscreen) {
        GetWindowPlacement(state->hwnd, &state->savedPlacement);
        state->style = static_cast<DWORD>(GetWindowLongPtrW(state->hwnd, GWL_STYLE));
        state->exStyle = static_cast<DWORD>(GetWindowLongPtrW(state->hwnd, GWL_EXSTYLE));

        MONITORINFO mi{sizeof(MONITORINFO)};
        GetMonitorInfoW(MonitorFromWindow(state->hwnd, MONITOR_DEFAULTTONEAREST), &mi);

        SetWindowLongPtrW(state->hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(state->hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        state->fullscreen = true;
    } else {
        SetWindowLongPtrW(state->hwnd, GWL_STYLE, state->style);
        SetWindowLongPtrW(state->hwnd, GWL_EXSTYLE, state->exStyle);
        SetWindowPlacement(state->hwnd, &state->savedPlacement);
        SetWindowPos(state->hwnd, nullptr, 0, 0, 0, 0,
            SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
        state->fullscreen = false;
    }
}

Uint2 WindowsPlatform::GetWindowSize(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        RECT client{};
        GetClientRect(state->hwnd, &client);
        return {static_cast<uint32_t>(client.right - client.left),
                static_cast<uint32_t>(client.bottom - client.top)};
    }
    return {};
}

Uint2 WindowsPlatform::GetWindowPixelSize(WindowId id) const {
    return GetWindowSize(id);
}

Int2 WindowsPlatform::GetWindowPosition(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        RECT rect{};
        GetWindowRect(state->hwnd, &rect);
        return {rect.left, rect.top};
    }
    return {};
}

float WindowsPlatform::GetWindowDpiScale(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        return state->dpiScale;
    }
    return 1.0f;
}

bool WindowsPlatform::IsWindowFocused(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        return GetForegroundWindow() == state->hwnd;
    }
    return false;
}

bool WindowsPlatform::IsWindowMaximized(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        return IsZoomed(state->hwnd) != FALSE;
    }
    return false;
}

bool WindowsPlatform::IsWindowMinimized(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        return IsIconic(state->hwnd) != FALSE;
    }
    return false;
}

std::vector<WindowId> WindowsPlatform::GetWindows() const {
    std::vector<WindowId> result;
    result.reserve(m_Windows.size());
    for (const auto& [id, _] : m_Windows) {
        result.push_back(static_cast<WindowId>(id));
    }
    return result;
}

void WindowsPlatform::SetWindowHitTest(WindowId id, WindowHitTestFn fn, void* userData) {
    if (WindowState* state = FindWindow(id)) {
        state->hitTest = fn;
        state->hitTestUserData = userData;
    }
}

bool WindowsPlatform::IsKeyDown(KeyCode key) const {
    const size_t index = static_cast<size_t>(key);
    return index < m_Keys.size() ? m_Keys[index] : false;
}

bool WindowsPlatform::IsMouseButtonDown(MouseButton button) const {
    const size_t index = static_cast<size_t>(button);
    return index < m_MouseButtons.size() ? m_MouseButtons[index] : false;
}

Int2 WindowsPlatform::GetMousePosition() const {
    POINT pt{};
    GetCursorPos(&pt);
    return {pt.x, pt.y};
}

Int2 WindowsPlatform::GetMousePosition(WindowId id) const {
    POINT pt{};
    GetCursorPos(&pt);
    if (const WindowState* state = FindWindow(id)) {
        ScreenToClient(state->hwnd, &pt);
    }
    return {pt.x, pt.y};
}

void WindowsPlatform::SetMousePosition(int32_t x, int32_t y) {
    SetCursorPos(x, y);
}

void WindowsPlatform::SetMousePosition(WindowId id, int32_t x, int32_t y) {
    if (WindowState* state = FindWindow(id)) {
        POINT pt{x, y};
        ClientToScreen(state->hwnd, &pt);
        SetCursorPos(pt.x, pt.y);
    }
}

void WindowsPlatform::SetRelativeMouseMode(WindowId id, bool enabled) {
    if (WindowState* state = FindWindow(id)) {
        state->relativeMouse = enabled;
        if (enabled) {
            RECT client{};
            GetClientRect(state->hwnd, &client);
            POINT ul{client.left, client.top};
            POINT lr{client.right, client.bottom};
            ClientToScreen(state->hwnd, &ul);
            ClientToScreen(state->hwnd, &lr);
            RECT clip{ul.x, ul.y, lr.x, lr.y};
            ClipCursor(&clip);
            SetCursorVisible(false);
        } else {
            ClipCursor(nullptr);
            SetCursorVisible(true);
        }
    }
}

bool WindowsPlatform::IsRelativeMouseMode(WindowId id) const {
    if (const WindowState* state = FindWindow(id)) {
        return state->relativeMouse;
    }
    return false;
}

void WindowsPlatform::SetCursorVisible(bool visible) {
    if (m_CursorVisible == visible) {
        return;
    }
    m_CursorVisible = visible;
    ShowCursor(visible ? TRUE : FALSE);
}

void WindowsPlatform::SetSystemCursor(SystemCursor cursor) {
    LPCWSTR id = IDC_ARROW;
    switch (cursor) {
    case SystemCursor::IBeam: id = IDC_IBEAM; break;
    case SystemCursor::Wait: id = IDC_WAIT; break;
    case SystemCursor::Crosshair: id = IDC_CROSS; break;
    case SystemCursor::Hand: id = IDC_HAND; break;
    case SystemCursor::SizeNS: id = IDC_SIZENS; break;
    case SystemCursor::SizeWE: id = IDC_SIZEWE; break;
    case SystemCursor::SizeNWSE: id = IDC_SIZENWSE; break;
    case SystemCursor::SizeNESW: id = IDC_SIZENESW; break;
    case SystemCursor::SizeAll: id = IDC_SIZEALL; break;
    case SystemCursor::No: id = IDC_NO; break;
    case SystemCursor::Hidden:
        SetCursorVisible(false);
        return;
    default: break;
    }
    m_CurrentCursor = LoadCursorW(nullptr, id);
    SetCursorVisible(true);
    SetCursor(m_CurrentCursor);
}

KeyModifier WindowsPlatform::GetKeyModifiers() const {
    return QueryModifiers();
}

} // namespace we::platform

#endif // WE_PLATFORM_WINDOWS
