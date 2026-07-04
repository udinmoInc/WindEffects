#include "EditorApplication.hpp"
#include "Core/Logger.hpp"

namespace we::editor::application {

EditorApplication::EditorApplication() = default;

EditorApplication::~EditorApplication() = default;

void EditorApplication::Initialize() {
    EngineApplication::Initialize();
}

void EditorApplication::Run() {
    EngineApplication::Run();
}

void EditorApplication::Shutdown() {
    EngineApplication::Shutdown();
}

void EditorApplication::OnUpdate(float /*deltaTime*/) {
}

void EditorApplication::OnRender() {
}

#if WE_HAS_SDL3
void EditorApplication::OnEvent(const SDL_Event& /*event*/) {
}
#endif

} // namespace we::editor::application
