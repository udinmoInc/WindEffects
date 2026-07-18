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

} // namespace we::platform
