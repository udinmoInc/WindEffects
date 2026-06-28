#include "EditorApplication.hpp"
#include "Core/Logger.hpp"

namespace we::editor::application {

EditorApplication::EditorApplication() {
}

EditorApplication::~EditorApplication() {
}

void EditorApplication::Initialize() {
    EngineApplication::Initialize();
    
    // Instead of initializing the full Legacy Editor inside Main, we do it here.
    // However, Legacy Editor handles its own window creation.
    // We would need to refactor Legacy Editor to not create its own window.
    // Given the complexity of Legacy Editor, for this phase we can initialize it directly.
    m_LegacyEditor = std::make_unique<Editor>(m_Window);
}

void EditorApplication::Run() {
    if (m_LegacyEditor) {
        m_LegacyEditor->Run();
    } else {
        EngineApplication::Run();
    }
}

void EditorApplication::Shutdown() {
    m_LegacyEditor.reset();
    EngineApplication::Shutdown();
}

void EditorApplication::OnUpdate(float deltaTime) {
    // Empty for now as Legacy Editor has its own loop
}

void EditorApplication::OnRender() {
    // Empty for now
}

void EditorApplication::OnEvent(const SDL_Event& event) {
    // Empty for now
}

} // namespace we::editor::application
