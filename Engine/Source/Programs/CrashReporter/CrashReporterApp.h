#pragma once
#include <SDL3/SDL.h>
#include <memory>

namespace we::runtime::renderer { class VulkanContext; class Renderer; class RenderGraph; }
namespace we::UI { class UIRenderer2; class EventSystem; }
namespace we::programs::crashreporter { class CrashReporterUI; }

namespace we::programs::crashreporter {

class CrashReporterApp {
public:
    CrashReporterApp(SDL_Window* window);
    ~CrashReporterApp();

    void Run();

private:
    void MainLoop();
    void Shutdown();

    SDL_Window* m_Window;
    bool m_Running = true;

    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::shared_ptr<we::runtime::renderer::RenderGraph> m_RenderGraph;
    std::unique_ptr<we::UI::UIRenderer2> m_UIRenderer;
    std::shared_ptr<we::UI::EventSystem> m_UIEventSystem;
    std::shared_ptr<CrashReporterUI> m_UI;
};

}
