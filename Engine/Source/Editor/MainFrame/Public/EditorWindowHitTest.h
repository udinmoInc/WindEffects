#pragma once

#include "MainFrame/Export.h"

#if WE_HAS_SDL3
#include <SDL3/SDL.h>
#endif
#include <memory>

namespace we::UI {
class TitleBar;
}

namespace we::editor::mainframe {

struct EditorWindowHitTestData {
    std::weak_ptr<we::UI::TitleBar> titleBar;
};

#if WE_HAS_SDL3
MAINFRAME_API SDL_HitTestResult SDLCALL EditorWindowHitTest(SDL_Window* window, const SDL_Point* area, void* userdata);
#endif

} // namespace we::editor::mainframe
