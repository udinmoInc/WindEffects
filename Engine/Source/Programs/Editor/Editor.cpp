#include "Editor.hpp"
#include "Core/Logger.hpp"
#include "Runtime/Core/ModuleManager.hpp"
#include "Runtime/Core/PluginManager.hpp"
#include "EditorRegistry.hpp"
#include "Widgets/Panel.hpp"
#include "Layout/Box.hpp"
#include "Layout/OverlayManager.hpp"
#include <iostream>

namespace we::programs::editor {

using namespace we::UI;
using namespace we::runtime::renderer;
using namespace we::runtime::scene;
using namespace we::runtime::engine;

Editor::Editor(SDL_Window* window) : m_Window(window) {
    HE_INFO("Initializing Vulkan Context...");
    m_Context = std::make_shared<VulkanContext>(m_Window);

    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());
    
    m_Renderer = std::make_shared<Renderer>(m_Context, m_Window);
    m_RenderGraph = std::make_shared<RenderGraph>(m_Renderer);
    m_SceneRenderer = std::make_shared<SceneRenderer>(m_Context, m_Renderer->GetOffscreenRenderPass(), m_Renderer->GetCameraDescLayout());
    m_GridRenderer = std::make_shared<GridRenderer>(m_Context, m_Renderer->GetOffscreenRenderPass(), m_Renderer->GetCameraDescLayout());

    m_Camera = std::make_shared<EditorCamera>();
    m_Scene = std::make_shared<Scene>(m_Context, m_SceneRenderer);
    m_Scene->InitializeDefaultScene(m_Renderer->GetCameraBuffer());

    m_UIRenderer = std::make_unique<UI::UIRenderer>();
    if (!m_UIRenderer->Init(m_Context, m_Renderer->GetSwapchainRenderPass())) {
        throw std::runtime_error("Failed to initialize HouseUI Renderer!");
    }
    m_UIEventSystem = std::make_shared<UI::EventSystem>();

    // Start all modules dynamically
    we::core::ModuleManager::Get().StartupAllModules();

    // Load plugins
    we::core::PluginManager::Get().ScanAndLoadPlugins("Plugins");

    BuildDynamicEditorUI();

    m_UIEventSystem->SetRootWidget(m_RootWidget);
    HE_INFO("WindEffects Engine Editor successfully bootstrapped dynamically.");
}

Editor::~Editor() {
    Shutdown();
}

void Editor::BuildDynamicEditorUI() {
    HE_INFO("Building Dynamic Editor UI from Registry");

    auto rootHBox = std::make_shared<UI::HorizontalBox>();
    rootHBox->SetSpacing(4.0f);

    // Fetch panels dynamically
    auto& panels = EditorRegistry::Get().GetPanels();
    for (const auto& [name, factory] : panels) {
        HE_INFO("Creating registered panel: " + name);
        if (auto panel = factory()) {
            rootHBox->AddChild(panel);
        }
    }

    auto overlayManager = std::make_shared<UI::OverlayManager>();
    overlayManager->SetBaseWidget(rootHBox);
    m_RootWidget = overlayManager;
}

void Editor::Run() {
    MainLoop();
}

void Editor::MainLoop() {
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (m_Running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window))) {
                m_Running = false;
            }

            // Simple event processing
            UI::MouseEvent mouseEvent{};
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseEvent.type = UI::MouseEventType::MouseMove;
                mouseEvent.position = UI::Point{ event.motion.x, event.motion.y };
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                mouseEvent.type = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? UI::MouseEventType::MouseDown : UI::MouseEventType::MouseUp;
                mouseEvent.position = UI::Point{ event.button.x, event.button.y };
                if (event.button.button == SDL_BUTTON_LEFT) mouseEvent.button = UI::MouseButton::Left;
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            }
        }

        if (!m_Running) break;

        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        m_Camera->Update(dt);
        m_Scene->Update();
        m_RootWidget->Tick(dt);

        we::runtime::renderer::Renderer::CameraUniform cameraUBO{};
        cameraUBO.view = m_Camera->GetViewMatrix();
        cameraUBO.proj = m_Camera->GetProjectionMatrix();
        cameraUBO.pos = m_Camera->GetPosition();
        cameraUBO.padding = 0.0f;
        m_Renderer->UpdateCameraBuffer(cameraUBO);

        if (m_Renderer->BeginFrame()) {
            VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();

            m_RenderGraph->BeginOffscreenPass(cmd);
            m_SceneRenderer->DrawSkybox(cmd, m_Renderer->GetCameraDescSet());
            m_GridRenderer->Draw(cmd, m_Renderer->GetCameraDescSet());
            m_RenderGraph->EndOffscreenPass(cmd);

            m_RenderGraph->BeginSwapchainPass(cmd);
            m_UIRenderer->Render(cmd, m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight(), m_RootWidget);
            m_RenderGraph->EndSwapchainPass(cmd);

            m_Renderer->EndFrame();
        }
    }
}

void Editor::Shutdown() {
    vkDeviceWaitIdle(m_Context->GetDevice());

    we::core::PluginManager::Get().UnloadAllPlugins();
    we::core::ModuleManager::Get().ShutdownAllModules();

    m_RootWidget.reset();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    m_UIEventSystem.reset();

    m_Scene.reset();
    m_Camera.reset();
    m_GridRenderer.reset();
    m_SceneRenderer.reset();
    m_RenderGraph.reset();
    m_Renderer.reset();
}

} // namespace we::programs::editor
