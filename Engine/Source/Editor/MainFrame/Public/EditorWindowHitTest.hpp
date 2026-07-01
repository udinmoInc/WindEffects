#pragma once

#include <SDL3/SDL.h>
#include <memory>

namespace we::UI {
class TitleBar;
}

namespace we::editor::mainframe {

struct EditorWindowHitTestData {
    std::weak_ptr<we::UI::TitleBar> titleBar;
};

SDL_HitTestResult SDLCALL EditorWindowHitTest(SDL_Window* window, const SDL_Point* area, void* userdata);

} // namespace we::editor::mainframe
