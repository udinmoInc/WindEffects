// ==============================================================================
// WindEffects — WeLauncher — WeLauncherApp
// Internal implementation for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "App/WeLauncherApp.h"

#include "App/LauncherContext.h"
#include "UI/Shell/LauncherHelpers.h"
#include "UI/Shell/LauncherLogo.h"
#include "UI/Shell/LauncherShell.h"
#include "Util/PathUtils.h"

#include "Renderer/Renderer.h"
#include "KindUI/Host/OverlayRenderer.h"
#include "KindUI/Host/OverlayRenderContext.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/LayoutIncremental.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Animator.h"
#include "Core/AssetRegistry.h"
#include "Core/Logger.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Core/ApplicationContext.h"
#include "KindUI/Core/WidgetContext.h"
#include "UI/Theming/LauncherTheme.h"
#include "KindUI/Theme/ThemeManager.h"
#include "KindUI/Core/DPIContext.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <variant>

namespace we::programs::welauncher {

we::platform::WindowHitTestResult WeLauncherApp::HitTestThunk(
    we::platform::WindowId,
    we::platform::Int2 point,
    void* userData) {
    auto* self = static_cast<WeLauncherApp*>(userData);
    if (!self || !self->m_UI) {
        return we::platform::WindowHitTestResult::Client;
    }
    return self->m_UI->WindowHitTest(point);
}

WeLauncherApp::WeLauncherApp(we::platform::WindowId window)
    : m_Window(window)
    , m_Context(std::make_shared<LauncherContext>()) {
    HE_INFO("[WeLauncher] Initializing");

    HE_INFO("[WeLauncher] Context Initialize...");
    m_Context->Initialize();
    HE_INFO("[WeLauncher] Context ready");

    m_AppContext = std::make_unique<we::runtime::kindui::ApplicationContext>(
        std::make_shared<LauncherTheme>());
    auto& platform = we::platform::Platform::Get();
    HE_INFO("[WeLauncher] Updating UI scale...");
    UpdateUiScaleFromWindow();
    const float dpiScale = we::runtime::kindui::DPIContext::GetScale();
    HE_INFO("[WeLauncher] AppContext Initialize...");
    m_AppContext->Initialize(dpiScale);

    HE_INFO("[WeLauncher] Presenter Init...");
    m_Presenter = std::make_shared<we::runtime::renderer::Renderer>();
    m_Presenter->Init(m_Window);
    HE_INFO("[WeLauncher] Loading editor assets...");
    we::core::AssetRegistry::Get().LoadDefaultEditorAssets();

    HE_INFO("[WeLauncher] OverlayRenderer Init...");
    m_UIRenderer = std::make_unique<we::runtime::kindui::OverlayRenderer>();
    if (m_Presenter->IsGpuReady()) {
        m_UIRenderer->Init(m_Presenter->GetRHIDevice(), m_Presenter->GetSwapchainFormat(), 2);
    } else {
        throw std::runtime_error("WeLauncher presenter initialized without a GPU-ready device.");
    }

    EnsureVisibleSwapchain();

    HE_INFO("[WeLauncher] Loading logo...");
    const int logoPx = static_cast<int>(std::round(kLauncherLogoDisplaySize * dpiScale));
    std::filesystem::path engineRoot;
    if (auto root = PathUtils::FindEngineRoot(PathUtils::GetExecutableDirectory())) {
        engineRoot = *root;
    }
    m_LogoSet = LoadLauncherLogoTexture(m_UIRenderer.get(), engineRoot, static_cast<uint32_t>(std::max(18, logoPx)));

    HE_INFO("[WeLauncher] Constructing shell...");
    m_WidgetContext = std::make_shared<we::runtime::kindui::WidgetContext>(*m_AppContext);

    m_UIEventSystem = std::make_shared<we::runtime::kindui::EventSystem>();
    m_UI = std::make_shared<LauncherShell>(m_Context, m_Window);
    m_UI->SetContext(m_WidgetContext);
    m_UI->SetHostContext(m_WidgetContext);
    m_UI->SetLogoTexture(m_LogoSet);
    m_UI->Construct(dpiScale);
    m_UIEventSystem->SetRootWidget(m_UI);

    if (m_Context->Settings().Settings().openLastProjectOnStart) {
        const auto& recent = m_Context->Settings().RecentProjects();
        if (!recent.empty()) {
            const auto result = m_Context->EditorLaunch().Launch(PathUtils::FromUtf8(recent.front()));
            if (result.success) {
                m_UI->SetStatus("Opened last project");
            } else if (!result.message.empty()) {
                m_UI->SetStatus(result.message);
            }
        }
    }

    platform.SetWindowHitTest(m_Window, &WeLauncherApp::HitTestThunk, this);
    we::runtime::kindui::UIRepaintGate::Request();
    HE_INFO("[WeLauncher] Ready");
}

WeLauncherApp::~WeLauncherApp() {
    Shutdown();
}

void WeLauncherApp::Run() {
    MainLoop();
}

void WeLauncherApp::UpdateUiScaleFromWindow() {
    float scale = 1.0f;
    if (m_Window != we::platform::WindowId::Invalid) {
        auto& platform = we::platform::Platform::Get();
        scale = platform.GetWindowDpiScale(m_Window);
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
    }

    const float clamped = std::clamp(scale, 1.0f, 3.0f);
    const float previous = we::runtime::kindui::DPIContext::GetScale();
    we::runtime::kindui::DPIContext::SetScale(clamped);
    if (std::abs(clamped - previous) > 0.001f || !m_DpiDiagnosticsLogged) {
        m_DpiDiagnosticsLogged = true;
        if (m_AppContext) {
            m_AppContext->Initialize(clamped);
        }
        if (m_UI && m_UIRenderer) {
            const int logoPx = static_cast<int>(std::round(kLauncherLogoDisplaySize * clamped));
            std::filesystem::path engineRoot;
            if (auto root = PathUtils::FindEngineRoot(PathUtils::GetExecutableDirectory())) {
                engineRoot = *root;
            }
            m_LogoSet = LoadLauncherLogoTexture(m_UIRenderer.get(), engineRoot, static_cast<uint32_t>(std::max(18, logoPx)));
            m_UI->SetLogoTexture(m_LogoSet);
        }
        we::runtime::kindui::UIRepaintGate::Request();

        if (m_Window != we::platform::WindowId::Invalid) {
            auto& platform = we::platform::Platform::Get();
            const auto physical = platform.GetWindowPixelSize(m_Window);
            const auto logicalX = static_cast<uint32_t>(std::round(static_cast<float>(physical.x) / clamped));
            const auto logicalY = static_cast<uint32_t>(std::round(static_cast<float>(physical.y) / clamped));
            const uint32_t dpi = static_cast<uint32_t>(std::round(clamped * 96.0f));
            const uint32_t swW = m_Presenter ? m_Presenter->GetSwapchainWidth() : 0;
            const uint32_t swH = m_Presenter ? m_Presenter->GetSwapchainHeight() : 0;

            const float contextScale = we::runtime::kindui::DPIContext::GetScale();
            const float themeScale = we::runtime::kindui::ThemeManager::Get().GetDpiScale();
            const char* consistency = (std::abs(contextScale - themeScale) < 0.001f && contextScale >= 1.0f) ? "PASS" : "FAIL";

            HE_INFO("[UI][GLOBAL DPI]\nDPI: " + std::to_string(dpi) +
                    "\nScale: " + std::to_string(clamped) +
                    "\nLogicalSize: " + std::to_string(logicalX) + "x" + std::to_string(logicalY) +
                    "\nPhysicalSize: " + std::to_string(physical.x) + "x" + std::to_string(physical.y) +
                    "\nFramebufferSize: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nScaleConsistency: " + std::string(consistency));

            HE_INFO("[UI][FRAMEBUFFER]\nWindowLogical: " + std::to_string(logicalX) + "x" + std::to_string(logicalY) +
                    "\nWindowPhysical: " + std::to_string(physical.x) + "x" + std::to_string(physical.y) +
                    "\nSwapchain: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nViewport: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nScissor: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nScale: " + std::to_string(clamped));

            const uint32_t fontAtlasPages = m_UIRenderer ? m_UIRenderer->GetFontAtlasPageCount() : 0;
            const uint32_t iconRasterSize = static_cast<uint32_t>(std::round(24.0f * clamped));
            HE_INFO("[UI][QUALITY]\nDPI: " + std::to_string(dpi) +
                    "\nScale: " + std::to_string(clamped) +
                    "\nLogical: " + std::to_string(logicalX) + "x" + std::to_string(logicalY) +
                    "\nPhysical: " + std::to_string(physical.x) + "x" + std::to_string(physical.y) +
                    "\nSwapchain: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nFontAtlas: " + std::to_string(fontAtlasPages) + " page(s)" +
                    "\nIconRasterSize: " + std::to_string(iconRasterSize) + "px" +
                    "\nFinalFramebuffer: " + std::to_string(swW) + "x" + std::to_string(swH));

            HE_INFO("[UI][QUALITY VERIFY]\nDPI: " + std::to_string(dpi) +
                    "\nScale: " + std::to_string(clamped) +
                    "\nLogicalSize: " + std::to_string(logicalX) + "x" + std::to_string(logicalY) +
                    "\nPhysicalSize: " + std::to_string(physical.x) + "x" + std::to_string(physical.y) +
                    "\nFramebufferSize: " + std::to_string(swW) + "x" + std::to_string(swH) +
                    "\nTextScale: " + std::to_string(clamped) +
                    "\nIconTier: " + (clamped > 1.1f ? "24px" : "16px") +
                    "\nRoundedRectAA: fwidth * 0.75" +
                    "\nBorderPhysicalWidth: " + std::to_string(std::max(1.0f, std::round(1.0f * clamped))) + "px" +
                    "\nScaleConsistency: " + std::string(consistency));
        }
    }
}

void WeLauncherApp::EnsureVisibleSwapchain() {
    if (!m_Presenter) {
        return;
    }
    auto& platform = we::platform::Platform::Get();
    auto pixelSize = platform.GetWindowPixelSize(m_Window);
    if (pixelSize.x == 0 || pixelSize.y == 0) {
        const auto logical = platform.GetWindowSize(m_Window);
        pixelSize = { logical.x, logical.y };
    }
    const int width = static_cast<int>(pixelSize.x);
    const int height = static_cast<int>(pixelSize.y);
    if (width > 0 && height > 0) {
        if (width != static_cast<int>(m_Presenter->GetSwapchainWidth()) ||
            height != static_cast<int>(m_Presenter->GetSwapchainHeight())) {
            m_Presenter->RecreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }
    }
}

void WeLauncherApp::SyncLayoutFromSwapchain() {
    if (!m_UI || !m_Presenter) {
        return;
    }

    const uint32_t w = m_Presenter->GetSwapchainWidth();
    const uint32_t h = m_Presenter->GetSwapchainHeight();
    if (w == 0 || h == 0) {
        return;
    }

    const bool sizeChanged = w != m_LastLayoutSwapchainW || h != m_LastLayoutSwapchainH;
    const bool needsFullLayout = sizeChanged || we::runtime::kindui::UIRepaintGate::ConsumeNeedsLayout();
    const bool needsOverlayLayout = !needsFullLayout
        && we::runtime::kindui::UIRepaintGate::ConsumeNeedsOverlayLayout();
    if (needsFullLayout) {
        using Size = we::runtime::kindui::Size;
        using Rect = we::runtime::kindui::Rect;
        we::runtime::kindui::LayoutIncrementalStats::ResetCurrent();
        ++we::runtime::kindui::LayoutIncrementalStats::Current().fullLayoutPasses;
        m_UI->Measure(Size{ static_cast<float>(w), static_cast<float>(h) });
        m_UI->Arrange(Rect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
        m_UI->ClearSubtreeLayoutDirty();
        we::runtime::kindui::UIRepaintGate::NotifyHostLayoutCompleted();
        m_UI->ReleaseRetainedPaintSubtree();
        we::runtime::kindui::UIRepaintGate::RequestPaintReason("LayoutSizeChanged");
        m_LastLayoutSwapchainW = w;
        m_LastLayoutSwapchainH = h;
    } else if (needsOverlayLayout) {
        m_UI->SyncOverlaysOnly();
        we::runtime::kindui::UIRepaintGate::RequestPaintReason("OverlayLayout");
    } else {
        (void)we::runtime::kindui::UIRepaintGate::ConsumeNeedsOverlayLayout();
    }
}

void WeLauncherApp::MainLoop() {
    auto& platform = we::platform::Platform::Get();
    uint64_t lastTime = platform.GetHighResolutionCounter();
    const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());
    we::runtime::kindui::UIRepaintGate::Request();

    while (m_Running) {
        if (!platform.PollEvents()) {
            m_Running = false;
        }

        for (const auto& event : platform.GetFrameEvents()) {
            bool requestUiRebuild = false;

            if (std::holds_alternative<we::platform::QuitEvent>(event)) {
                m_Running = false;
            } else if (const auto* close = std::get_if<we::platform::WindowCloseEvent>(&event)) {
                if (close->window == m_Window) {
                    m_Running = false;
                }
            } else if (std::holds_alternative<we::platform::WindowResizeEvent>(event)
                || std::holds_alternative<we::platform::WindowMaximizeEvent>(event)
                || std::holds_alternative<we::platform::WindowMinimizeEvent>(event)) {
                EnsureVisibleSwapchain();
                if (m_UI) {
                    m_UI->OnWindowStateChanged();
                }
                requestUiRebuild = true;
            } else if (const auto* dpi = std::get_if<we::platform::WindowDpiEvent>(&event)) {
                if (dpi->window == m_Window) {
                    UpdateUiScaleFromWindow();
                    EnsureVisibleSwapchain();
                    if (m_UI) {
                        m_UI->OnWindowStateChanged();
                    }
                    requestUiRebuild = true;
                }
            } else if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = we::runtime::kindui::MouseEventType::MouseMove;
                mouseEvent.position = { static_cast<float>(move->position.x), static_cast<float>(move->position.y) };
                const auto mods = platform.GetKeyModifiers();
                mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiRebuild = true;
            } else if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = button->pressed
                    ? we::runtime::kindui::MouseEventType::MouseDown
                    : we::runtime::kindui::MouseEventType::MouseUp;
                mouseEvent.position = { static_cast<float>(button->position.x),
                    static_cast<float>(button->position.y) };
                switch (button->button) {
                case we::platform::MouseButton::Left: mouseEvent.button = we::runtime::kindui::MouseButton::Left; break;
                case we::platform::MouseButton::Right: mouseEvent.button = we::runtime::kindui::MouseButton::Right;
                    break;
                case we::platform::MouseButton::Middle: mouseEvent.button = we::runtime::kindui::MouseButton::Middle;
                    break;
                default: break;
                }
                mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
                mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
                mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiRebuild = true;
            } else if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
                we::runtime::kindui::MouseEvent mouseEvent{};
                mouseEvent.type = we::runtime::kindui::MouseEventType::MouseWheel;
                mouseEvent.position = { static_cast<float>(wheel->position.x), static_cast<float>(wheel->position.y) };
                mouseEvent.wheelDeltaX = wheel->delta.x;
                mouseEvent.wheelDeltaY = wheel->delta.y;
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
                requestUiRebuild = true;
            } else if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
                we::runtime::kindui::KeyEvent keyEvent{};
                keyEvent.type = key->pressed
                    ? we::runtime::kindui::KeyEventType::KeyDown
                    : we::runtime::kindui::KeyEventType::KeyUp;
                keyEvent.key = key->key;
                keyEvent.altDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Alt);
                keyEvent.shiftDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Shift);
                keyEvent.ctrlDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Control);
                if (key->pressed) {
                    m_UI->OnKeyDown(keyEvent);
                }
                m_UIEventSystem->ProcessKeyEvent(keyEvent);
                requestUiRebuild = true;
            } else if (const auto* text = std::get_if<we::platform::TextInputEvent>(&event)) {
                if (m_UI && text->window == m_Window && text->codepoint != 0) {
                    m_UI->OnTextInput(text->codepoint);
                }
                requestUiRebuild = true;
            }

            if (requestUiRebuild) {
                we::runtime::kindui::UIRepaintGate::Request();
            }
        }

        if (!m_Running) {
            break;
        }

        const uint64_t now = platform.GetHighResolutionCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) {
            dt = 0.1f;
        }

        we::runtime::kindui::Animator::Tick(dt);
        const bool needsWidgetTick = we::runtime::kindui::UIRepaintGate::PeekNeedsWidgetTick();
        if (needsWidgetTick) {
            we::runtime::kindui::UIRepaintGate::MarkSettled();
            m_UI->Tick(dt);
        }
        UpdateUiScaleFromWindow();
        SyncLayoutFromSwapchain();

        // After minimize the swapchain may briefly report zero; retry recreate once ready.
        if (m_Presenter->GetSwapchainWidth() == 0 || m_Presenter->GetSwapchainHeight() == 0) {
            EnsureVisibleSwapchain();
        }

        if (m_Presenter->BeginFrame()) {
            m_UIRenderer->SetTargetExtent(
                m_Presenter->GetSwapchainWidth(),
                m_Presenter->GetSwapchainHeight());

            we::runtime::uigfx::OverlayRenderContext context;
            context.cmd = m_Presenter->GetFrameCommandList();
            context.targetView = we::rhi::RHITextureViewHandle::Invalid;
            context.targetFormat = m_Presenter->GetSwapchainFormat();
            context.targetExtent = { m_Presenter->GetSwapchainWidth(), m_Presenter->GetSwapchainHeight() };

            m_UIRenderer->BeginOverlayPass(context);
            m_UIRenderer->RenderUI(m_UI, m_Presenter->GetCurrentFrameIndex());
            m_UIRenderer->EndOverlayPass(context);

            m_Presenter->SubmitAndPresent();
        } else {
            // Swapchain not ready (e.g. mid-resize) â€” keep requesting layout for next frame.
            we::runtime::kindui::UIRepaintGate::Request();
        }
    }

    m_Context->Save();
}

void WeLauncherApp::Shutdown() {
    auto& platform = we::platform::Platform::Get();
    platform.SetWindowHitTest(m_Window, nullptr, nullptr);

    if (m_AppContext) {
        m_AppContext->Shutdown();
        m_AppContext.reset();
    }
    m_WidgetContext.reset();
    m_UIEventSystem.reset();
    m_UI.reset();
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    if (m_Presenter) {
        m_Presenter->Shutdown();
        m_Presenter.reset();
    }
}

} // namespace we::programs::welauncher
