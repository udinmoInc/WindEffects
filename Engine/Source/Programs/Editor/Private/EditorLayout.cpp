// ==============================================================================
// WindEffects — Editor — EditorLayout
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Editor.h"
#include "Core/DiagnosticMacros.h"
#include "Core/Logger.h"
#include "Environment/EnvironmentEditorApi.h"
#include "Environment/EnvironmentSystem.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Profiling/ScreenRecorder.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Theming/ThemeManager.h"
#include "Platform/InputTypes.h"
#include "Platform/PlatformSDK.h"
#include "Widgets/ViewportWidget.h"

#include <algorithm>

#include "Platform/UndefWin32Macros.h"

namespace {
void MarkOsInput(we::runtime::kindui::UiInteractionKind kind) {
    if (we::runtime::kindui::UiInputLatencyAudit::IsEnabled()) {
        we::runtime::kindui::UiInputLatencyAudit::Get().OnOsEvent(kind);
    }
}
} // namespace

namespace we::programs::editor {

using ::we::editor::viewport::ViewportWidget;

namespace UI = we::runtime::kindui;
using namespace we::runtime::kindui;

void Editor::UpdateUiScaleFromWindow() {
    float scale = 1.0f;
    if (m_Window != we::platform::WindowId::Invalid) {
        auto& platform = we::platform::Platform::Get();
        // Layout uses client-area pixels (GetClientRect). DPI scale is independent of
        // outer vs client size — never derive scale from pixel/logical size ratios on Windows
        // where both APIs return the client rect.
        scale = platform.GetWindowDpiScale(m_Window);
        if (scale <= 0.0f) {
            scale = 1.0f;
        }
    }

    const float clamped = std::clamp(scale, 1.0f, 3.0f);
    const float previous = we::runtime::kindui::DPIContext::GetScale();
    if (std::abs(clamped - previous) <= 0.001f) {
        return;
    }

    we::runtime::kindui::DPIContext::SetScale(clamped);
    if (we::runtime::kindui::ThemeManager::Get().IsInitialized()) {
        we::runtime::kindui::ThemeManager::Get().SetDpiScale(clamped);
    }
    we::runtime::kindui::UIRepaintGate::Request();
}

void Editor::EnsureVisibleSwapchain() {
    // Nested focus/resize/command callbacks collapse into one pass.
    if (m_EnsureSwapchainInProgress) {
        m_EnsureSwapchainPending = true;
        HE_DEBUG("[Render] EnsureVisibleSwapchain coalesced (in-progress)");
        return;
    }

    if (!m_Renderer) {
        return;
    }

    m_EnsureSwapchainInProgress = true;
    do {
        m_EnsureSwapchainPending = false;

        auto& platform = we::platform::Platform::Get();
        if (platform.IsWindowMinimized(m_Window)) {
            m_ForceSwapchainRecreate = false;
            break;
        }
        auto pixelSize = platform.GetWindowPixelSize(m_Window);
        if (pixelSize.x == 0 || pixelSize.y == 0) {
            const auto logical = platform.GetWindowSize(m_Window);
            pixelSize = {logical.x, logical.y};
        }

        const int width = static_cast<int>(pixelSize.x);
        const int height = static_cast<int>(pixelSize.y);

        if (width <= 0 || height <= 0) {
            HE_ERROR("[Render] Window still reports zero size — UI layout empty until resized.");
            m_ForceSwapchainRecreate = false;
            break;
        }

        // Recreate swapchain when size changes or when force-requested (focus/restore).
        const bool sizeChanged =
            width != static_cast<int>(m_Renderer->GetSwapchainWidth()) ||
            height != static_cast<int>(m_Renderer->GetSwapchainHeight());
        const bool forceRecreate = m_ForceSwapchainRecreate;

        if (sizeChanged || forceRecreate) {
            HE_INFO("[Render] Ensuring swapchain matches visible window (" + std::to_string(width) + "x" +
                std::to_string(height) + ") force=" + std::to_string(forceRecreate ? 1 : 0) + "...");
            m_Renderer->RecreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            HE_INFO("[Render] Swapchain recreated for visible window.");
            m_HasRenderedScene = false;
            m_LastLayoutSwapchainW = 0;
            m_LastLayoutSwapchainH = 0;
            m_ForceSwapchainRecreate = false;
            we::runtime::kindui::UIRepaintGate::Request();
        } else {
            // Duplicate focus/tab activation with identical extent — skip recreate.
            m_ForceSwapchainRecreate = false;
        }
    } while (m_EnsureSwapchainPending);

    m_EnsureSwapchainInProgress = false;
    m_EnsureSwapchainPending = false;
}

bool Editor::SyncViewportFramebufferFromLayout() {
    bool layoutOrResize = false;
    if (!m_RootWidget || !m_Renderer) {
        return false;
    }

    const uint32_t w = m_Renderer->GetSwapchainWidth();
    const uint32_t h = m_Renderer->GetSwapchainHeight();
    if (w == 0 || h == 0) {
        return false;
    }

    const bool sizeChanged = w != m_LastLayoutSwapchainW || h != m_LastLayoutSwapchainH;
    const bool needsLayout = sizeChanged || we::runtime::kindui::UIRepaintGate::ConsumeNeedsLayout();

    if (needsLayout) {
        // Root Measure/Arrange uses the swapchain = Windows CLIENT RECT only
        // (GetClientRect). Title/menu/toolbar/status are flex chrome rows; the
        // workspace Column FlexGrow(1) receives the remaining client area.
        const UI::Rect clientRect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) };
        we::runtime::kindui::UiPathDiagnostics::Get().OnLayoutPass();
        m_RootWidget->Measure(UI::Size{ clientRect.width, clientRect.height });
        m_RootWidget->Arrange(clientRect);
        m_RootWidget->ClearSubtreeLayoutDirty();
        we::runtime::kindui::UIRepaintGate::RequestPaint();
        we::runtime::kindui::UiInputLatencyAudit::Get().OnLayout();
        m_LastLayoutSwapchainW = w;
        m_LastLayoutSwapchainH = h;
        layoutOrResize = true;
    }

    if (m_ViewportWidget) {
        if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_ViewportWidget)) {
            // New viewport targets are empty until RenderScene fills them — never paint-only after resize.
            if (vp->FlushPendingResize()) {
                m_HasRenderedScene = false;
                layoutOrResize = true;
            }
            vp->SyncRendererViewport();
        }
    }
    return layoutOrResize;
}

void Editor::LogWidgetTreeLayout(const std::shared_ptr<UI::Widget>& widget, const std::string& name, int depth) {
    if (!widget) {
        HE_ERROR("[UI] Widget tree node '" + name + "' is null.");
        return;
    }

    const uint32_t w = m_Renderer->GetSwapchainWidth();
    const uint32_t h = m_Renderer->GetSwapchainHeight();
    widget->Measure(Size{ static_cast<float>(w), static_cast<float>(h) });
    widget->Arrange(Rect{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });

    const Rect geom = widget->GetGeometry();

    if (geom.width <= 0.0f || geom.height <= 0.0f) {
        std::string indent(depth * 2, ' ');
        HE_ERROR("[UI] " + indent + name + " has ZERO size - will not paint visible content.");
    }

    for (size_t i = 0; i < widget->GetChildren().size(); ++i) {
        LogWidgetTreeLayout(widget->GetChildren()[i], name + ".child[" + std::to_string(i) + "]", depth + 1);
    }
}

void Editor::CreateNewLevel() {
    if (!m_Scene) {
        HE_ERROR("[Editor] New Level failed — scene is null.");
        return;
    }

    m_Scene->Clear();
    if (m_Scene->IsEmpty()) {
        we::runtime::world::environment::EnvironmentSystem::Get().EnsureDefaultEnvironment();
    }
    ::we::editor::environment::TickEditor();
    m_HasRenderedScene = false;
    we::runtime::kindui::UIRepaintGate::Request();
    HE_INFO("[Editor] New Level — scene cleared.");
    if (we::runtime::kindui::ScreenRecorder::IsRecordingEnabled()) {
        we::runtime::kindui::ScreenRecorder::Get().RecordEvent("[Editor] New Level");
    }
}

void Editor::ProcessLateInputMouse() {
    if (!m_UIEventSystem || !m_RootWidget) {
        return;
    }
    auto& platform = we::platform::Platform::Get();
    const auto pos = platform.GetMousePosition(m_Window);
    if (pos.x == m_LastSampledMousePos.x && pos.y == m_LastSampledMousePos.y) {
        return;
    }
    m_LastSampledMousePos = pos;

    UI::MouseEvent mouseEvent{};
    mouseEvent.type = UI::MouseEventType::MouseMove;
    mouseEvent.position = UI::Point{ static_cast<float>(pos.x), static_cast<float>(pos.y) };
    const auto mods = platform.GetKeyModifiers();
    mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
    mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
    mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
    MarkOsInput(UI::UiInteractionKind::MouseMove);
    m_UIEventSystem->ProcessMouseEvent(mouseEvent);
}

} // namespace we::programs::editor
