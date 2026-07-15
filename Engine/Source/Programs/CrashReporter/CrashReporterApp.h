#pragma once

#include "Platform/Types.h"
#include <memory>

namespace we::runtime::renderer { class Renderer; }
namespace WindEffects::Editor::UI { class OverlayRenderer; class EventSystem; }
namespace we::programs::crashreporter { class CrashReporterUI; }

namespace we::programs::crashreporter {

class CrashReporterApp {
public:
    explicit CrashReporterApp(we::platform::WindowId window);
    ~CrashReporterApp();

    void Run();

private:
    void MainLoop();
    void Shutdown();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    bool m_Running = true;

    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::unique_ptr<WindEffects::Editor::UI::OverlayRenderer> m_UIRenderer;
    std::shared_ptr<WindEffects::Editor::UI::EventSystem> m_UIEventSystem;
    std::shared_ptr<CrashReporterUI> m_UI;
};

}
