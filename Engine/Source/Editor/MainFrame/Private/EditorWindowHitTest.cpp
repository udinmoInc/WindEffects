#include "EditorWindowHitTest.hpp"
#include "Widgets/TitleBar.hpp"

namespace we::editor::mainframe {

#if WE_HAS_SDL3
SDL_HitTestResult SDLCALL EditorWindowHitTest(SDL_Window* window, const SDL_Point* area, void* userdata) {
    if (!window || !area) {
        return SDL_HITTEST_NORMAL;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);

    constexpr int kResizeBorder = 8;
    const bool left = area->x < kResizeBorder;
    const bool right = area->x >= width - kResizeBorder;
    const bool top = area->y < kResizeBorder;
    const bool bottom = area->y >= height - kResizeBorder;

    if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (left) return SDL_HITTEST_RESIZE_LEFT;
    if (right) return SDL_HITTEST_RESIZE_RIGHT;
    if (top) return SDL_HITTEST_RESIZE_TOP;
    if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;

    if (userdata) {
        auto* data = static_cast<EditorWindowHitTestData*>(userdata);
        if (auto titleBar = data->titleBar.lock()) {
            return titleBar->HitTest(*area);
        }
    }

    return SDL_HITTEST_NORMAL;
}
#endif

} // namespace we::editor::mainframe
