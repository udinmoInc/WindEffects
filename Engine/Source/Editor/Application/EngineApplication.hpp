#pragma once

#include <volk.h>
#include <SDL3/SDL.h>
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

    SDL_Window* GetWindow() const { return m_Window; }
    void Quit() { m_Running = false; }

protected:
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnEvent(const SDL_Event& event) {}

    SDL_Window* m_Window = nullptr;
    bool m_Running = false;

private:
    void InitWindow();
    static SDL_HitTestResult SDLCALL HitTestCallback(SDL_Window* win, const SDL_Point* area, void* data);
};

} // namespace we::editor::application
