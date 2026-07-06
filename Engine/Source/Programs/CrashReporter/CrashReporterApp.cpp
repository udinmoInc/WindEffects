#include "CrashReporterApp.hpp"
#include "CrashReporterUI.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/Renderer.hpp"
#include "Renderer/Shader/ShaderLibrary.hpp"
#include "Renderer/RenderGraph.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Core/EventSystem.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include "Core/Logger.hpp"
#include <filesystem>

namespace we::programs::crashreporter {

CrashReporterApp::CrashReporterApp(SDL_Window* window) : m_Window(window) {
    HE_INFO("[CrashReporterApp] Constructor started");
    
    HE_INFO("[CrashReporterApp] Creating VulkanContext");
    m_Context = std::make_shared<we::runtime::renderer::VulkanContext>(m_Window);
    HE_INFO("[CrashReporterApp] VulkanContext created");

    HE_INFO("[CrashReporterApp] Creating Renderer");
    m_Renderer = std::make_shared<we::runtime::renderer::Renderer>(m_Context, m_Window);
    m_RenderGraph = std::make_shared<we::runtime::renderer::RenderGraph>(m_Renderer);
    HE_INFO("[CrashReporterApp] Renderer created");

    HE_INFO("[CrashReporterApp] Initializing ShaderLibrary");
    std::string shaderRoot = "Engine/Shaders";
    std::string bytecodeRoot = "Assets/Shaders";
    for (const char* candidate : {"Engine/Shaders", "../Engine/Shaders", "../../Engine/Shaders"})
    {
        if (std::filesystem::exists(candidate))
        {
            shaderRoot = candidate;
            break;
        }
    }
    for (const char* candidate : {
             "Engine/Shaders/Bytecodes",
             "Assets/Shaders",
             "../Assets/Shaders",
             "Shaders"})
    {
        if (std::filesystem::exists(candidate))
        {
            bytecodeRoot = candidate;
            break;
        }
    }
    we::runtime::renderer::ShaderLibrary::Get().Initialize(shaderRoot, bytecodeRoot);
    HE_INFO("[CrashReporterApp] ShaderLibrary initialized");

    HE_INFO("[CrashReporterApp] Loading default editor assets");
    we::core::AssetRegistry::Get().LoadDefaultEditorAssets();
    HE_INFO("[CrashReporterApp] Default editor assets loaded");

    HE_INFO("[CrashReporterApp] Creating UIRenderer");
    m_UIRenderer = std::make_unique<we::UI::UIRenderer>();
    HE_INFO("[CrashReporterApp] Initializing UIRenderer");
    m_UIRenderer->Init(m_Context, m_Renderer->GetSwapchainRenderPass());
    HE_INFO("[CrashReporterApp] UIRenderer initialized");
    
    HE_INFO("[CrashReporterApp] Creating EventSystem");
    m_UIEventSystem = std::make_shared<we::UI::EventSystem>();
    HE_INFO("[CrashReporterApp] EventSystem created");

    HE_INFO("[CrashReporterApp] Creating CrashReporterUI");
    m_UI = std::make_shared<CrashReporterUI>();
    HE_INFO("[CrashReporterApp] Constructing UI");
    m_UI->Construct();
    HE_INFO("[CrashReporterApp] UI constructed");

    HE_INFO("[CrashReporterApp] Setting root widget");
    m_UIEventSystem->SetRootWidget(m_UI);
    HE_INFO("[CrashReporterApp] Constructor completed");
}

CrashReporterApp::~CrashReporterApp() {
    Shutdown();
}

void CrashReporterApp::Run() {
    HE_INFO("[CrashReporterApp] Run() called, starting MainLoop");
    MainLoop();
    HE_INFO("[CrashReporterApp] MainLoop() returned");
}

void CrashReporterApp::MainLoop() {
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (m_Running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || 
               (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window))) {
                m_Running = false;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION ||
                event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
                event.type == SDL_EVENT_MOUSE_BUTTON_UP ||
                event.type == SDL_EVENT_MOUSE_WHEEL) {

                we::UI::MouseEvent mouseEvent{};
                mouseEvent.position = we::UI::Point{ static_cast<float>(event.motion.x), static_cast<float>(event.motion.y) };

                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    mouseEvent.type = we::UI::MouseEventType::MouseMove;
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    mouseEvent.type = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                        ? we::UI::MouseEventType::MouseDown
                        : we::UI::MouseEventType::MouseUp;
                    mouseEvent.position = we::UI::Point{ static_cast<float>(event.button.x), static_cast<float>(event.button.y) };

                    if (event.button.button == SDL_BUTTON_LEFT)   mouseEvent.button = we::UI::MouseButton::Left;
                    else if (event.button.button == SDL_BUTTON_RIGHT)  mouseEvent.button = we::UI::MouseButton::Right;
                    else if (event.button.button == SDL_BUTTON_MIDDLE) mouseEvent.button = we::UI::MouseButton::Middle;
                } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    mouseEvent.type = we::UI::MouseEventType::MouseWheel;
                    mouseEvent.wheelDeltaX = static_cast<float>(event.wheel.x);
                    mouseEvent.wheelDeltaY = static_cast<float>(event.wheel.y);
                }

                const SDL_Keymod mods = SDL_GetModState();
                mouseEvent.altDown   = (mods & SDL_KMOD_ALT) != 0;
                mouseEvent.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
                mouseEvent.ctrlDown  = (mods & SDL_KMOD_CTRL) != 0;

                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                we::UI::KeyEvent keyEvent{};
                keyEvent.type = (event.type == SDL_EVENT_KEY_DOWN)
                    ? we::UI::KeyEventType::KeyDown
                    : we::UI::KeyEventType::KeyUp;
                keyEvent.keycode = event.key.key;

                const SDL_Keymod mods = event.key.mod;
                keyEvent.altDown   = (mods & SDL_KMOD_ALT) != 0;
                keyEvent.shiftDown = (mods & SDL_KMOD_SHIFT) != 0;
                keyEvent.ctrlDown  = (mods & SDL_KMOD_CTRL) != 0;

                m_UIEventSystem->ProcessKeyEvent(keyEvent);
            }
        }

        if (!m_Running) break;

        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        m_UI->Tick(dt);

        if (m_Renderer->BeginFrame()) {
            VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();

            m_UIRenderer->PrepareFrame(
                m_Renderer->GetSwapchainWidth(),
                m_Renderer->GetSwapchainHeight(),
                m_Renderer->GetCurrentFrameIndex(),
                m_UI);
            m_RenderGraph->BeginSwapchainPass(cmd);
            m_UIRenderer->RecordDrawCommands(
                cmd,
                m_Renderer->GetSwapchainWidth(),
                m_Renderer->GetSwapchainHeight(),
                m_Renderer->GetCurrentFrameIndex());
            m_RenderGraph->EndSwapchainPass(cmd);

            m_Renderer->EndFrame();
        }
    }
}

void CrashReporterApp::Shutdown() {
    if (m_Context) {
        m_Context->WaitUntilIdle();
    }
    m_UIEventSystem.reset();
    m_UI.reset();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_RenderGraph.reset();
        m_UIRenderer.reset();
    }
    m_Renderer.reset();
    m_Context.reset();
}

}
