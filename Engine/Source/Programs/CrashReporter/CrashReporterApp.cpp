#include "CrashReporterApp.h"
#include "CrashReporterUI.h"
#include "Runtime/Renderer/Public/Core/DeviceContext.h"
#include "Runtime/Renderer/Public/Renderer/Renderer.h"
#include "Runtime/Renderer/Public/Shader/ShaderLibrary.h"
#include "Runtime/Renderer/Public/Graph/RenderGraph.h"
#include "Rendering/OverlayRenderer.h"
#include "Rendering/OverlayRenderContext.h"
#include "Core/EventSystem.h"
#include "Runtime/Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Platform/PlatformSDK.h"
#include <filesystem>
#include <variant>

namespace we::programs::crashreporter {

CrashReporterApp::CrashReporterApp(we::platform::WindowId window) : m_Window(window) {
    HE_INFO("[CrashReporterApp] Constructor started");

    const auto native = we::platform::Platform::Get().GetNativeWindowHandle(m_Window);

    we::runtime::renderer::DeviceContextConfig deviceConfig;
    deviceConfig.nativeWindow = native;
    deviceConfig.appName = "CrashReporter";
    deviceConfig.enableValidationLayers = false;
    m_Context = std::make_shared<we::runtime::renderer::DeviceContext>();
    m_Context->Init(deviceConfig);

    m_Renderer = std::make_shared<we::runtime::renderer::Renderer>();
    m_Renderer->Init(m_Window);
    m_RenderGraph = std::make_shared<we::runtime::renderer::RenderGraph>();
    m_RenderGraph->Init(m_Renderer.get());

    std::string shaderRoot = "Engine/Shaders";
    std::string bytecodeRoot = "Assets/Shaders";
    for (const char* candidate : {"Engine/Shaders", "../Engine/Shaders", "../../Engine/Shaders"}) {
        if (std::filesystem::exists(candidate)) {
            shaderRoot = candidate;
            break;
        }
    }
    for (const char* candidate : {
             "Engine/Shaders/Bytecodes", "Assets/Shaders", "../Assets/Shaders", "Shaders"}) {
        if (std::filesystem::exists(candidate)) {
            bytecodeRoot = candidate;
            break;
        }
    }
    we::runtime::renderer::ShaderLibrary::Get().Initialize(shaderRoot, bytecodeRoot);
    we::core::AssetRegistry::Get().LoadDefaultEditorAssets();

    m_UIRenderer = std::make_unique<WindEffects::Editor::UI::OverlayRenderer>();
    m_UIRenderer->Init(
        m_Context->GetPhysicalDevice(),
        m_Context->GetDevice(),
        m_Context->GetGraphicsQueue(),
        m_Context->GetGraphicsQueueFamily(),
        m_Renderer->GetSwapchainImageFormat(),
        2,
        m_Context.get(),
        m_Renderer->GetResourceManager());

    m_UIEventSystem = std::make_shared<WindEffects::Editor::UI::EventSystem>();
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
                WindEffects::Editor::UI::MouseEvent mouseEvent{};
                mouseEvent.type = WindEffects::Editor::UI::MouseEventType::MouseMove;
                mouseEvent.position = {static_cast<float>(move->position.x), static_cast<float>(move->position.y)};
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                WindEffects::Editor::UI::MouseEvent mouseEvent{};
                mouseEvent.type = button->pressed
                    ? WindEffects::Editor::UI::MouseEventType::MouseDown
                    : WindEffects::Editor::UI::MouseEventType::MouseUp;
                mouseEvent.position = {static_cast<float>(button->position.x), static_cast<float>(button->position.y)};
                switch (button->button) {
                case we::platform::MouseButton::Left: mouseEvent.button = WindEffects::Editor::UI::MouseButton::Left; break;
                case we::platform::MouseButton::Right: mouseEvent.button = WindEffects::Editor::UI::MouseButton::Right; break;
                case we::platform::MouseButton::Middle: mouseEvent.button = WindEffects::Editor::UI::MouseButton::Middle; break;
                default: break;
                }
                mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                WindEffects::Editor::UI::MouseEvent mouseEvent{};
                mouseEvent.type = WindEffects::Editor::UI::MouseEventType::MouseWheel;
                mouseEvent.position = {static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y)};
                mouseEvent.wheelDeltaX = wheel->delta.x;
                mouseEvent.wheelDeltaY = wheel->delta.y;
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            } else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
                WindEffects::Editor::UI::KeyEvent keyEvent{};
                keyEvent.type = key->pressed
                    ? WindEffects::Editor::UI::KeyEventType::KeyDown
                    : WindEffects::Editor::UI::KeyEventType::KeyUp;
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
            VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();

            m_UIRenderer->SetTargetExtent(
                m_Renderer->GetSwapchainWidth(),
                m_Renderer->GetSwapchainHeight());

            we::editor::rendering::OverlayRenderContext context;
            context.cmd = cmd;
            context.targetView = VK_NULL_HANDLE;
            context.targetFormat = m_Renderer->GetSwapchainImageFormat();
            context.targetExtent = { m_Renderer->GetSwapchainWidth(), m_Renderer->GetSwapchainHeight() };

            m_UIRenderer->BeginOverlayPass(context);
            m_UIRenderer->RenderEditorUI(m_UI, m_Renderer->GetCurrentFrameIndex());
            m_UIRenderer->EndOverlayPass(context);

            m_Renderer->SubmitAndPresent();
        }
    }
}

void CrashReporterApp::Shutdown() {
    if (m_Context) {
        m_Context->Shutdown();
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
