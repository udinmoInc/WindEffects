#pragma once

#if WE_HAS_SDL3
#include <SDL3/SDL.h>
#endif
#include <memory>
#include <string>

namespace we::editor::application {

class EngineApplication {
public:
    EngineApplication();
    virtual ~EngineApplication();

    EngineApplication(const EngineApplication&) = delete;
    EngineApplication& operator=(const EngineApplication&) = delete;

    virtual void Initialize();
    void Run();
    virtual void Shutdown();

#if WE_HAS_SDL3
    SDL_Window* GetWindow() const { return m_Window; }
#else
    void* GetWindow() const { return m_Window; }
#endif
    void Quit() { m_Running = false; }

protected:
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
#if WE_HAS_SDL3
    virtual void OnEvent(const SDL_Event& event) {}
#else
    virtual void OnEvent(void* event) {}
#endif

#if WE_HAS_SDL3
    SDL_Window* m_Window = nullptr;
#else
    void* m_Window = nullptr;
#endif
    bool m_Running = false;

private:
    void InitWindow();
#if WE_HAS_SDL3
    static SDL_HitTestResult SDLCALL HitTestCallback(SDL_Window* win, const SDL_Point* area, void* data);
#endif
};

} // namespace we::editor::application
