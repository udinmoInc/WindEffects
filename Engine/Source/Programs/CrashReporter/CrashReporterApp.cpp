#include "CrashReporterApp.hpp"
#include "CrashReporterUI.hpp"
#include "Renderer/VulkanContext.hpp"
#include "Renderer/Renderer.hpp"
#include "Rendering/UIRenderer.hpp"
#include "Core/EventSystem.hpp"
#include "Runtime/Core/AssetRegistry.hpp"
#include "Core/Logger.hpp"

namespace we::programs::crashreporter {

CrashReporterApp::CrashReporterApp(SDL_Window* window) : m_Window(window) {
    m_Context = std::make_shared<we::runtime::renderer::VulkanContext>(m_Window);
    volkInitialize();
    volkLoadInstance(m_Context->GetInstance());
    volkLoadDevice(m_Context->GetDevice());

    m_Renderer = std::make_shared<we::runtime::renderer::Renderer>(m_Context, m_Window);

    we::core::AssetRegistry::Get().LoadDefaultEditorAssets();

    m_UIRenderer = std::make_unique<we::UI::UIRenderer>();
    m_UIRenderer->Init(m_Context, m_Renderer->GetSwapchainRenderPass());
    m_UIEventSystem = std::make_shared<we::UI::EventSystem>();

    m_UI = std::make_shared<CrashReporterUI>();
    m_UI->Construct();

    m_UIEventSystem->SetRootWidget(m_UI);
}

CrashReporterApp::~CrashReporterApp() {
    Shutdown();
}

void CrashReporterApp::Run() {
    MainLoop();
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

            we::UI::MouseEvent mouseEvent{};
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseEvent.type = we::UI::MouseEventType::MouseMove;
                mouseEvent.position = we::UI::Point{ (float)event.motion.x, (float)event.motion.y };
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                mouseEvent.type = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? we::UI::MouseEventType::MouseDown : we::UI::MouseEventType::MouseUp;
                mouseEvent.position = we::UI::Point{ (float)event.button.x, (float)event.button.y };
                if (event.button.button == SDL_BUTTON_LEFT) mouseEvent.button = we::UI::MouseButton::Left;
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
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
            
            int w, h;
            SDL_GetWindowSizeInPixels(m_Window, &w, &h);
            
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_Renderer->GetSwapchainRenderPass();
            renderPassInfo.framebuffer = m_Renderer->GetSwapchainFramebuffer(m_Renderer->GetCurrentImageIndex());
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = { static_cast<uint32_t>(w), static_cast<uint32_t>(h) };

            VkClearValue clearColor = {{{0.015f, 0.015f, 0.015f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            m_UIRenderer->Render(cmd, static_cast<uint32_t>(w), static_cast<uint32_t>(h), m_UI);
            vkCmdEndRenderPass(cmd);

            m_Renderer->EndFrame();
        }
    }
}

void CrashReporterApp::Shutdown() {
    if (m_Context) {
        vkDeviceWaitIdle(m_Context->GetDevice());
    }
    m_UIEventSystem.reset();
    m_UI.reset();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    m_Renderer.reset();
}

}
