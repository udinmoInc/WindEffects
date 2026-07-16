#pragma once

#include "Platform/Types.h"
#include <memory>

namespace we::runtime::renderer { class Renderer; }
namespace WindEffects::Editor::UI {
class OverlayRenderer;
class EventSystem;
class EditorApplicationContext;
}

namespace we::programs::welauncher {
class LauncherContext;
class LauncherShell;
}

namespace we::programs::welauncher {

class WeLauncherApp {
public:
    explicit WeLauncherApp(we::platform::WindowId window);
    ~WeLauncherApp();

    void Run();

private:
    void MainLoop();
    void Shutdown();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    bool m_Running = true;

    std::shared_ptr<LauncherContext> m_Context;
    std::unique_ptr<WindEffects::Editor::UI::EditorApplicationContext> m_AppContext;
    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::unique_ptr<WindEffects::Editor::UI::OverlayRenderer> m_UIRenderer;
    std::shared_ptr<WindEffects::Editor::UI::EventSystem> m_UIEventSystem;
    std::shared_ptr<LauncherShell> m_UI;
};

} // namespace we::programs::welauncher
