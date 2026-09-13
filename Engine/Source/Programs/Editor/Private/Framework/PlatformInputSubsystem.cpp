// ==============================================================================
// WindEffects — Editor — PlatformInputSubsystem
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/PlatformInputSubsystem.h"

#include "Core/DiagnosticMacros.h"
#include "Core/LoopExecutionTrace.h"
#include <KindUI/EditorUI.h>
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "Platform/Events.h"
#include "Platform/InputTypes.h"
#include "Platform/PlatformSDK.h"
#include "Widgets/ViewportWidget.h"

#include <sstream>
#include <string>

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {
namespace {

using ::we::runtime::kindui::MouseButton;
using ::we::editor::viewport::ViewportWidget;
namespace UI = we::runtime::kindui;

UI::UiInteractionKind InteractionKindForMouse(const UI::MouseEvent& event) {
    switch (event.type) {
    case UI::MouseEventType::MouseMove: return UI::UiInteractionKind::MouseMove;
    case UI::MouseEventType::MouseWheel: return UI::UiInteractionKind::Wheel;
    case UI::MouseEventType::MouseDown: return UI::UiInteractionKind::Click;
    case UI::MouseEventType::MouseUp: return UI::UiInteractionKind::ButtonPress;
    default: return UI::UiInteractionKind::Unknown;
    }
}

UI::UiInteractionKind InteractionKindForKey(bool pressed) {
    (void)pressed;
    return UI::UiInteractionKind::Keyboard;
}

void MarkOsInput(UI::UiInteractionKind kind) {
    if (UI::UiInputLatencyAudit::IsEnabled()) {
        UI::UiInputLatencyAudit::Get().OnOsEvent(kind);
    }
}

} // namespace

PlatformInputSubsystem::PlatformInputSubsystem(
    IEditorLoopHost& host,
    we::runtime::core::ApplicationFramework& framework,
    EditorLoopComponentRegistry& components)
    : m_Host(host)
    , m_Framework(framework)
    , m_Components(components) {}

void PlatformInputSubsystem::Initialize() {
    HE_INFO("[PlatformInputSubsystem] Initialized");
}

void PlatformInputSubsystem::Tick(float /*deltaTime*/) {
    // Event routing happens in ProcessFrameEvents() during ProcessPlatformEvents.
}

void PlatformInputSubsystem::Shutdown() {
    HE_INFO("[PlatformInputSubsystem] Shutdown");
}

bool PlatformInputSubsystem::ProcessFrameEvents() {
    we::runtime::core::LoopExecutionTrace::Scoped scope("PlatformInput.ProcessFrameEvents");
    auto& platform = we::platform::Platform::Get();
    {
        we::runtime::core::LoopExecutionTrace::Scoped pollScope("PlatformInput.PollEvents");
        if (!platform.PollEvents()) {
            we::runtime::core::LoopExecutionTrace::Event(
                "PlatformInput.PollEvents", "returned false (quit / m_WantsQuit)");
            m_Host.RequestHostStop();
            if (auto overlay = m_Host.GetHostOverlayHost()) {
                overlay->ExecutePendingCallbacks();
            }
            return false;
        }
    }

    const auto frameEvents = platform.GetFrameEvents();
    m_LastEventCount = frameEvents.size();
    UI::UiInputLatencyAudit::Get().BeginFrame(static_cast<uint32_t>(frameEvents.size()));
    we::runtime::core::LoopExecutionTrace::GateState(
        "PlatformInput.afterPoll",
        UI::UIRepaintGate::PeekNeedsLayout(),
        UI::UIRepaintGate::PeekNeedsPaint(),
        frameEvents.size());

    // Keep hit-test geometry current before routing pointer/keyboard input.
    m_Host.HostUpdateUiScaleFromWindow();
    if (UI::UIRepaintGate::PeekNeedsLayout()) {
        m_Host.HostSyncViewportFramebufferFromLayout();
    }

    // Two-pass routing: deliver all pointer/keyboard events before lifecycle side
    // effects (focus clear / swapchain arm). Windows often queues WM_SETFOCUS with
    // the activating click in one PeekMessage batch; clearing capture between
    // MouseDown and MouseUp left header ToolButtons in an orphaned pressed=false
    // state so OnMouseUp never fired OnClicked.
    bool requestUiLayout = false;
    bool requestUiPaint = false;

    for (const auto& event : frameEvents) {
        bool paint = false;
        HandlePointerKeyboard(event, paint);
        requestUiPaint = requestUiPaint || paint;
        m_Components.PlatformEvent(m_Host, event);
    }

    for (const auto& event : frameEvents) {
        bool layout = false;
        bool paint = false;
        HandleWindowLifecycle(event, layout, paint);
        requestUiLayout = requestUiLayout || layout;
        requestUiPaint = requestUiPaint || paint;
    }

    if (requestUiLayout) {
        UI::UIRepaintGate::Request();
        we::runtime::core::LoopExecutionTrace::Event("UIRepaintGate.Request", "layout+paint from lifecycle/input");
    } else if (requestUiPaint) {
        UI::UIRepaintGate::RequestPaint();
        we::runtime::core::LoopExecutionTrace::Event("UIRepaintGate.RequestPaint", "paint from pointer/key");
    }

    if (auto overlay = m_Host.GetHostOverlayHost(); overlay && overlay->HasOpenPopups()) {
        overlay->ExecutePendingCallbacks();
    }
    m_Components.PostInput(m_Host);

    we::runtime::core::LoopExecutionTrace::GateState(
        "PlatformInput.postInput",
        UI::UIRepaintGate::PeekNeedsLayout(),
        UI::UIRepaintGate::PeekNeedsPaint(),
        m_LastEventCount);

    return m_Host.IsHostRunning();
}

void PlatformInputSubsystem::ClearUiInputCapture() {
    // Capture → OnCaptureLost clears ToolButton pressed/modal state before any
    // deferred swapchain work, so a later MouseUp cannot fire an orphaned click.
    if (auto ui = m_Host.GetHostUIEventSystem()) {
        ui->ClearAllInputState();
    }
    // Drop transient chrome popups that would otherwise keep an orphaned modal
    // hit-target after alt-tab / minimize without re-entering the message pump.
    if (auto overlay = m_Host.GetHostOverlayHost(); overlay && overlay->HasOpenPopups()) {
        overlay->CloseTransientPopups();
    }
}

void PlatformInputSubsystem::RequestDeferredSwapchainRefresh() {
    // Arm force-recreate and enqueue. Never call EnsureVisibleSwapchain here —
    // that would block inside PollEvents/DispatchMessage (fence wait re-entrancy).
    // CommandQueue coalesces duplicate SwapchainRecreateRequest; ProcessCommand
    // CAS-dedups the NeedsRecreation flag. Always push so an in-flight Ensure
    // that only flipped Force cannot leave recreate stranded until the next focus.
    m_Host.HostForceSwapchainRecreate() = true;
    m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
        we::runtime::core::ApplicationCommand::Type::SwapchainRecreateRequest));
}

void PlatformInputSubsystem::HandleWindowLifecycle(
    const we::platform::PlatformEvent& event,
    bool& requestUiLayout,
    bool& requestUiPaint) {
    auto& platform = we::platform::Platform::Get();
    const auto window = m_Host.GetHostWindow();

    if (std::holds_alternative<we::platform::QuitEvent>(event)) {
        we::runtime::core::LoopExecutionTrace::Event(
            "Lifecycle.QuitEvent", "RequestHostStop + ShutdownRequest command");
        ClearUiInputCapture();
        m_Host.RequestHostStop();
        if (auto overlay = m_Host.GetHostOverlayHost()) {
            overlay->ExecutePendingCallbacks();
        }
        m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
            we::runtime::core::ApplicationCommand::Type::ShutdownRequest));
        return;
    }

    if (const auto* close = std::get_if<we::platform::WindowCloseEvent>(&event)) {
        if (close->window == window) {
            we::runtime::core::LoopExecutionTrace::Event(
                "Lifecycle.WindowCloseEvent", "toolbar/OS close → RequestHostStop + ShutdownRequest");
            ClearUiInputCapture();
            m_Host.RequestHostStop();
            if (auto overlay = m_Host.GetHostOverlayHost()) {
                overlay->ExecutePendingCallbacks();
            }
            m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
                we::runtime::core::ApplicationCommand::Type::ShutdownRequest));
        }
        return;
    }

    if (const auto* minEv = std::get_if<we::platform::WindowMinimizeEvent>(&event)) {
        if (minEv->window == window) {
            // Always clear capture first — ShowWindow(SW_MINIMIZE) may nest focus/size
            // messages; never leave a pressed toolbar button owning capture across them.
            ClearUiInputCapture();
            if (minEv->minimized) {
                we::runtime::core::LoopExecutionTrace::Event(
                    "Lifecycle.WindowMinimize", "minimized=1 → clear input + WindowMinimized cmd");
                m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
                    we::runtime::core::ApplicationCommand::Type::WindowMinimized));
            } else {
                we::runtime::core::LoopExecutionTrace::Event(
                    "Lifecycle.WindowMinimize", "minimized=0 → deferred SwapchainRecreateRequest");
                RequestDeferredSwapchainRefresh();
                m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
                    we::runtime::core::ApplicationCommand::Type::WindowRestored));
            }
        }
        requestUiLayout = true;
        m_Host.HostUpdateUiScaleFromWindow();
        m_Host.HostSyncViewportFramebufferFromLayout();
        return;
    }

    if (std::holds_alternative<we::platform::WindowResizeEvent>(event)
        || std::holds_alternative<we::platform::WindowDpiEvent>(event)
        || std::holds_alternative<we::platform::WindowMaximizeEvent>(event)) {
        // Defer Ensure to SwapchainSubsystem/RenderPipeline so the message pump is not
        // blocked inside DispatchMessage/event iteration (avoids fence wait re-entrancy).
        if (!platform.IsWindowMinimized(window)) {
            RequestDeferredSwapchainRefresh();
        }
        requestUiLayout = true;
        m_Host.HostUpdateUiScaleFromWindow();
        m_Host.HostSyncViewportFramebufferFromLayout();
        return;
    }

    if (const auto* cursor = std::get_if<we::platform::CursorEnterEvent>(&event)) {
        if (cursor->window == window && !cursor->entered) {
            ClearUiInputCapture();
            requestUiPaint = true;
        }
        return;
    }

    if (const auto* focus = std::get_if<we::platform::WindowFocusEvent>(&event)) {
        if (focus->window != window) {
            return;
        }
        if (!focus->focused) {
            // Focus-lost only: drop capture/hover so alt-tab cannot leave a pressed
            // ToolButton owning the click. Never clear on focus-gained — that would
            // abort the activating click that brought the window forward.
            ClearUiInputCapture();
            HE_INFO("[Loop] focus=lost");
            m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
                we::runtime::core::ApplicationCommand::Type::WindowFocusLost));
        } else {
            requestUiLayout = true;
            HE_INFO("[Loop] focus=gained - swapchain refresh");
            RequestDeferredSwapchainRefresh();
            m_Framework.PushCommand(we::runtime::core::ApplicationCommand(
                we::runtime::core::ApplicationCommand::Type::WindowFocusGained));
        }
    }
}

void PlatformInputSubsystem::HandlePointerKeyboard(
    const we::platform::PlatformEvent& event,
    bool& requestUiPaint) {
    auto& platform = we::platform::Platform::Get();
    auto ui = m_Host.GetHostUIEventSystem();
    if (!ui) {
        return;
    }

    if (const auto* move = std::get_if<we::platform::MouseMoveEvent>(&event)) {
        if (UI::UIRepaintGate::PeekNeedsLayout()) {
            m_Host.HostSyncViewportFramebufferFromLayout();
        }
        UI::MouseEvent mouseEvent{};
        mouseEvent.type = UI::MouseEventType::MouseMove;
        mouseEvent.position = UI::Point{ static_cast<float>(move->position.x),
            static_cast<float>(move->position.y) };
        mouseEvent.deltaX = move->delta.x;
        mouseEvent.deltaY = move->delta.y;
        const auto mods = platform.GetKeyModifiers();
        mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
        mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
        mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
        MarkOsInput(InteractionKindForMouse(mouseEvent));
        ui->ProcessMouseEvent(mouseEvent);
        m_Host.HostLastSampledMousePos() = move->position;
        return;
    }

    if (const auto* raw = std::get_if<we::platform::RawMouseEvent>(&event)) {
        if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(m_Host.GetHostViewportWidget());
            vp && vp->IsFlyLookActive()) {
            UI::MouseEvent mouseEvent{};
            mouseEvent.type = UI::MouseEventType::MouseMove;
            const auto pos = platform.GetMousePosition(m_Host.GetHostWindow());
            mouseEvent.position = UI::Point{ static_cast<float>(pos.x), static_cast<float>(pos.y) };
            mouseEvent.deltaX = raw->delta.x;
            mouseEvent.deltaY = raw->delta.y;
            const auto mods = platform.GetKeyModifiers();
            mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
            mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
            mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
            MarkOsInput(UI::UiInteractionKind::MouseMove);
            ui->ProcessMouseEvent(mouseEvent);
            m_Host.HostLastSampledMousePos() = pos;
        }
        return;
    }

    if (const auto* button = std::get_if<we::platform::MouseButtonEvent>(&event)) {
        if (UI::UIRepaintGate::PeekNeedsLayout()) {
            m_Host.HostSyncViewportFramebufferFromLayout();
        }
        UI::MouseEvent mouseEvent{};
        mouseEvent.type = button->pressed ? UI::MouseEventType::MouseDown : UI::MouseEventType::MouseUp;
        mouseEvent.position = UI::Point{ static_cast<float>(button->position.x),
            static_cast<float>(button->position.y) };
        switch (button->button) {
        case we::platform::MouseButton::Left: mouseEvent.button = MouseButton::Left; break;
        case we::platform::MouseButton::Right: mouseEvent.button = MouseButton::Right; break;
        case we::platform::MouseButton::Middle: mouseEvent.button = MouseButton::Middle; break;
        default: mouseEvent.button = MouseButton::None; break;
        }
        mouseEvent.altDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Alt);
        mouseEvent.shiftDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Shift);
        mouseEvent.ctrlDown = we::platform::HasFlag(button->modifiers, we::platform::KeyModifier::Control);
        if (!button->pressed && button->button == we::platform::MouseButton::Left) {
            std::ostringstream detail;
            detail << "MouseUp L @(" << button->position.x << "," << button->position.y
                   << ") gateL=" << (UI::UIRepaintGate::PeekNeedsLayout() ? 1 : 0)
                   << " gateP=" << (UI::UIRepaintGate::PeekNeedsPaint() ? 1 : 0)
                   << " minimized=" << (platform.IsWindowMinimized(m_Host.GetHostWindow()) ? 1 : 0);
            we::runtime::core::LoopExecutionTrace::Event("Input.ToolbarClickCandidate", detail.str());
        }
        MarkOsInput(InteractionKindForMouse(mouseEvent));
        {
            we::runtime::core::LoopExecutionTrace::Scoped clickScope(
                button->pressed ? "KindUI.ProcessMouseDown" : "KindUI.ProcessMouseUp");
            ui->ProcessMouseEvent(mouseEvent);
        }
        requestUiPaint = true;
        return;
    }

    if (const auto* wheel = std::get_if<we::platform::MouseWheelEvent>(&event)) {
        if (UI::UIRepaintGate::PeekNeedsLayout()) {
            m_Host.HostSyncViewportFramebufferFromLayout();
        }
        UI::MouseEvent mouseEvent{};
        mouseEvent.type = UI::MouseEventType::MouseWheel;
        mouseEvent.position = UI::Point{ static_cast<float>(wheel->position.x),
            static_cast<float>(wheel->position.y) };
        mouseEvent.wheelDeltaX = wheel->delta.x;
        mouseEvent.wheelDeltaY = wheel->delta.y;
        const auto mods = platform.GetKeyModifiers();
        mouseEvent.altDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Alt);
        mouseEvent.shiftDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Shift);
        mouseEvent.ctrlDown = we::platform::HasFlag(mods, we::platform::KeyModifier::Control);
        MarkOsInput(UI::UiInteractionKind::Wheel);
        ui->ProcessMouseEvent(mouseEvent);
        requestUiPaint = true;
        return;
    }

    if (const auto* key = std::get_if<we::platform::KeyEvent>(&event)) {
        UI::KeyEvent keyEvent{};
        keyEvent.type = key->pressed ? UI::KeyEventType::KeyDown : UI::KeyEventType::KeyUp;
        keyEvent.key = key->key;
        keyEvent.altDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Alt);
        keyEvent.shiftDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Shift);
        keyEvent.ctrlDown = we::platform::HasFlag(key->modifiers, we::platform::KeyModifier::Control);
        MarkOsInput(InteractionKindForKey(key->pressed));
        ui->ProcessKeyEvent(keyEvent);
        requestUiPaint = true;
        return;
    }

    if (const auto* text = std::get_if<we::platform::TextInputEvent>(&event)) {
        ui->ProcessTextInput(text->codepoint);
        requestUiPaint = true;
    }
}

} // namespace we::programs::editor
