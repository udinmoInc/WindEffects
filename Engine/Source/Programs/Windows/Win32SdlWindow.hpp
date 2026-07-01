#pragma once

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL.h>

namespace we::programs::windows {

inline HWND GetHwndFromSdlWindow(SDL_Window* window) {
    if (!window) {
        return nullptr;
    }

    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
    if (!props) {
        return nullptr;
    }

    return static_cast<HWND>(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
}

} // namespace we::programs::windows

#endif // _WIN32
