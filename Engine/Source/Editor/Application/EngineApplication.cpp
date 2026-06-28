#include "EngineApplication.hpp"
#include "Core/Logger.hpp"
#include <stdexcept>
#include <chrono>

namespace we::editor::application {

EngineApplication::EngineApplication() {
}

EngineApplication::~EngineApplication() {
    Shutdown();
}

void EngineApplication::Initialize() {
    InitWindow();
    m_Running = true;
}

void EngineApplication::InitWindow() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to initialize SDL");
    }

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_VULKAN |
        SDL_WINDOW_RESIZABLE |
        SDL_WINDOW_HIGH_PIXEL_DENSITY |
        SDL_WINDOW_HIDDEN |
        SDL_WINDOW_BORDERLESS
    );

    m_Window = SDL_CreateWindow(
        "WindEffects Engine",
        1920, 1080,
        window_flags
    );

    if (!m_Window) {
        throw std::runtime_error("Failed to create SDL window");
    }

    SDL_SetWindowHitTest(m_Window, HitTestCallback, this);
    SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_Window);
}

void EngineApplication::Shutdown() {
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
    SDL_Quit();
}

void EngineApplication::Run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_Running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_Running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window)) {
                m_Running = false;
            }
            OnEvent(event);
        }

        OnUpdate(deltaTime);
        OnRender();
    }
}

SDL_HitTestResult SDLCALL EngineApplication::HitTestCallback(SDL_Window* win, const SDL_Point* area, void* data) {
    // Basic hit test logic for borderless window resizing
    int w, h;
    SDL_GetWindowSize(win, &w, &h);
    const int resizeBorder = 8;
    const int titleBarHeight = 32;

    bool left = area->x < resizeBorder;
    bool right = area->x >= w - resizeBorder;
    bool top = area->y < resizeBorder;
    bool bottom = area->y >= h - resizeBorder;

    if (top && left) return SDL_HITTEST_RESIZE_TOPLEFT;
    if (top && right) return SDL_HITTEST_RESIZE_TOPRIGHT;
    if (bottom && left) return SDL_HITTEST_RESIZE_BOTTOMLEFT;
    if (bottom && right) return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
    if (left) return SDL_HITTEST_RESIZE_LEFT;
    if (right) return SDL_HITTEST_RESIZE_RIGHT;
    if (top) return SDL_HITTEST_RESIZE_TOP;
    if (bottom) return SDL_HITTEST_RESIZE_BOTTOM;

    if (area->y < titleBarHeight) {
        return SDL_HITTEST_DRAGGABLE;
    }

    return SDL_HITTEST_NORMAL;
}

} // namespace we::editor::application
