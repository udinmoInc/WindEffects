#pragma once

#include "EngineApplication.hpp"
#include <memory>
#include "Editor.hpp" // For now, wrapping the existing Editor

namespace HouseEngine {

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
    void OnEvent(const SDL_Event& event) override;

private:
    std::unique_ptr<Editor> m_LegacyEditor;
};

} // namespace HouseEngine
