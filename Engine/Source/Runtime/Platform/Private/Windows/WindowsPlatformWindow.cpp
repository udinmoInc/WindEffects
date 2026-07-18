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

} // namespace we::platform
