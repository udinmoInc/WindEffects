#pragma once

#include "EngineApplication.hpp"

namespace we::editor::application {

class EditorApplication : public EngineApplication {
public:
    EditorApplication();
    ~EditorApplication() override;

    void Initialize() override;
    void Run();
    void Shutdown() override;

protected:
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
#if WE_HAS_SDL3
    void OnEvent(const SDL_Event& event) override;
#endif
};

} // namespace we::editor::application
