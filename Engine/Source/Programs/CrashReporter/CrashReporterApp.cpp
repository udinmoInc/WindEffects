#include "CrashReporterApp.h"
#include "CrashReporterUI.h"
#include "Renderer/Renderer.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "KindUI/Rendering/OverlayRenderContext.h"
#include "KindUI/Core/EventSystem.h"
#include "Runtime/Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Platform/PlatformSDK.h"
#include <variant>

namespace we::programs::crashreporter {

CrashReporterApp::CrashReporterApp(we::platform::WindowId window) : m_Window(window) {
    HE_INFO("[CrashReporterApp] Constructor started");

    m_Renderer = std::make_shared<we::runtime::renderer::Renderer>();
    m_Renderer->Init(m_Window);
    we::core::AssetRegistry::Get().LoadDefaultEditorAssets();

    m_UIRenderer = std::make_unique<we::runtime::kindui::OverlayRenderer>();
    if (m_Renderer->IsGpuReady()) {
        m_UIRenderer->Init(m_Renderer->GetRHIDevice(), m_Renderer->GetSwapchainFormat(), 2);
    }

    m_UIEventSystem = std::make_shared<we::runtime::kindui::EventSystem>();
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
    auto& platform = we::platform::Platform::Get();
    uint64_t lastTime = platform.GetHighResolutionCounter();
    const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());

    while (m_Running) {
        if (!platform.PollEvents()) {
            m_Running = false;
        }

        for (const auto& event : platform.GetFrameEvents()) {
            if (std::holds_alternative<we::platform::QuitEvent>(event)) {
                m_Running = false;
            } else if (const auto* close = std::get_if<we::platform::WindowCloseEvent>(&event)) {
                if (close->window == m_Window) {
                    m_Running = false;
                }
            } else if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = we::runtime::kindui::MouseEventType::MouseMove;
                mouseEvent.position = {static_cast<float>(move->position.x), static_cast<float>(move->position.y)};
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = button->pressed
                    ? we::runtime::kindui::MouseEventType::MouseDown
                    : we::runtime::kindui::MouseEventType::MouseUp;
                mouseEvent.position = {static_cast<float>(button->position.x), static_cast<float>(button->position.y)};
                switch (button->button) {
                case we::platform::MouseButton::Left: mouseEvent.button = we::runtime::kindui::MouseButton::Left; break;
                case we::platform::MouseButton::Right: mouseEvent.button = we::runtime::kindui::MouseButton::Right; break;
                case we::platform::MouseButton::Middle: mouseEvent.button = we::runtime::kindui::MouseButton::Middle; break;
                default: break;
                }
                mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = we::runtime::kindui::MouseEventType::MouseWheel;
                mouseEvent.position = {static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y)};
                mouseEvent.wheelDeltaX = wheel->delta.x;
                mouseEvent.wheelDeltaY = wheel->delta.y;
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
                we::runtime::kindui::KeyEvent keyEvent{};
                keyEvent.type = key->pressed
                    ? we::runtime::kindui::KeyEventType::KeyDown
                    : we::runtime::kindui::KeyEventType::KeyUp;
                keyEvent.key = key->key;
                keyEvent.altDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Alt);
                keyEvent.shiftDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Shift);
                keyEvent.ctrlDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessKeyEvent(keyEvent);
            }
        }

        if (!m_Running) break;

        uint64_t now = platform.GetHighResolutionCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        m_UI->Tick(dt);

        if (m_Renderer->BeginFrame()) {
            m_UIRenderer->SetTargetExtent(
                m_Renderer->GetSwapchainWidth(),
                m_Renderer->GetSwapchainHeight());

            we::editor::rendering::OverlayRenderContext context;
            context.cmd = m_Renderer->GetFrameCommandList();
            context.targetView = we::rhi::RHITextureViewHandle::Invalid;
            context.targetFormat = m_Renderer->GetSwapchainFormat();
            context.targetExtent = { m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight() };

            m_UIRenderer->BeginOverlayPass(context);
            m_UIRenderer->RenderEditorUI(m_UI, m_Renderer->GetCurrentFrameIndex());
            m_UIRenderer->EndOverlayPass(context);

            m_Renderer->SubmitAndPresent();
        }
    }
}

void CrashReporterApp::Shutdown() {
    m_UIEventSystem.reset();
    m_UI.reset();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    if (m_Renderer) {
        m_Renderer->Shutdown();
        m_Renderer.reset();
    }
}

}
