#pragma once

#include "Platform/Types.h"
#include "RHI/Types.h"

#include <memory>

namespace WindEffects::Editor::UI {
class OverlayRenderer;
class EventSystem;
class EditorApplicationContext;
class IWidgetContext;
}

namespace we::runtime::renderer { class Renderer; }

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
    void UpdateUiScaleFromWindow();
    void SyncLayoutFromSwapchain();
    void EnsureVisibleSwapchain();

    static we::platform::WindowHitTestResult HitTestThunk(
        we::platform::WindowId window,
        we::platform::Int2 point,
        void* userData);

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    bool m_Running = true;

    std::shared_ptr<LauncherContext> m_Context;
    std::unique_ptr<WindEffects::Editor::UI::EditorApplicationContext> m_AppContext;
    std::shared_ptr<WindEffects::Editor::UI::IWidgetContext> m_WidgetContext;
    std::shared_ptr<we::runtime::renderer::Renderer> m_Presenter;
    std::unique_ptr<WindEffects::Editor::UI::OverlayRenderer> m_UIRenderer;
    std::shared_ptr<WindEffects::Editor::UI::EventSystem> m_UIEventSystem;
    std::shared_ptr<LauncherShell> m_UI;

    uint32_t m_LastLayoutSwapchainW = 0;
    uint32_t m_LastLayoutSwapchainH = 0;
    we::rhi::RHIDescriptorSetHandle m_LogoSet = we::rhi::RHIDescriptorSetHandle::Invalid;
};

} // namespace we::programs::welauncher
