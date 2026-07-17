#include "ViewportNavigation.h"
#include "WindEffects/Editor/UI/Shell/ViewportNavigationSettings.h"
#include "ViewportToolbarState.h"
#include "Platform/Platform.h"

#include <algorithm>

#include "KindUI/Input/InputEvents.h"
#include "Platform/Platform.h"

namespace we::editor::viewport {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;

namespace {

we::runtime::engine::EditorCameraNavigationSettings ToCameraSettings(const ::we::editor::viewport::ViewportNavigationSettings& settings) {
    we::runtime::engine::EditorCameraNavigationSettings result{};
    result.mouseSensitivity = settings.mouseSensitivity;
    result.cameraAcceleration = settings.cameraAcceleration;
    result.cameraSmoothing = settings.cameraSmoothing;
    result.invertX = settings.invertX;
    result.invertY = settings.invertY;
    result.maxBoostMultiplier = settings.maxBoostMultiplier;
    result.slowMultiplier = settings.slowMultiplier;
    result.scrollWheelSpeedMultiplier = settings.scrollWheelSpeedMultiplier;
    return result;
}

} // namespace

void ApplyViewportNavigationSettings(
    const std::shared_ptr<we::runtime::engine::EditorCamera>& camera) {
    if (!camera) {
        return;
    }

    auto& store = ::we::editor::viewport::ViewportNavigationSettingsStore::Get();
    store.EnsureLoaded();
    camera->SetNavigationSettings(ToCameraSettings(store.GetSettings()));
    camera->SetCameraSpeed(store.GetSettings().defaultCameraSpeed);
    we::programs::editor::UpdateViewportCameraSpeedIndicator();
}

void ViewportNavigationController::ApplySettingsFromStore() {
    ApplyViewportNavigationSettings(m_Camera);
}

bool ViewportNavigationController::IsPointerInsideViewport(const Point& position) const {
    return m_ViewportRect.width > 0.0f
        && m_ViewportRect.height > 0.0f
        && m_ViewportRect.Contains(position);
}

glm::vec3 ViewportNavigationController::ResolveOrbitPivot() const {
    auto& store = ::we::editor::viewport::ViewportNavigationSettingsStore::Get();
    store.EnsureLoaded();

    if (store.GetSettings().orbitAroundSelection && m_Scene) {
        const int selectedIndex = m_Scene->GetSelectedEntityIndex();
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_Scene->GetEntities().size())) {
            return m_Scene->GetEntities()[static_cast<size_t>(selectedIndex)].Position;
        }
    }

    if (m_Camera) {
        return m_Camera->GetOrbitPivot();
    }
    return glm::vec3(0.0f);
}

void ViewportNavigationController::SaveCursorPosition(const Point& position) {
    m_SavedCursorPos = position;
    m_HasSavedCursorPos = true;
}

void ViewportNavigationController::RestoreCursorPosition() {
    if (m_Window == we::platform::WindowId::Invalid || !m_HasSavedCursorPos) {
        return;
    }
    we::platform::Platform::Get().SetMousePosition(
        m_Window,
        static_cast<int32_t>(m_SavedCursorPos.x),
        static_cast<int32_t>(m_SavedCursorPos.y));
}

void ViewportNavigationController::CenterFlyCursor() {
    if (m_Window == we::platform::WindowId::Invalid || m_ViewportRect.width <= 0.0f || m_ViewportRect.height <= 0.0f) {
        return;
    }

    const float centerX = m_ViewportRect.x + m_ViewportRect.width * 0.5f;
    const float centerY = m_ViewportRect.y + m_ViewportRect.height * 0.5f;
    we::platform::Platform::Get().SetMousePosition(
        m_Window,
        static_cast<int32_t>(centerX),
        static_cast<int32_t>(centerY));
}

void ViewportNavigationController::ApplySystemCursor(ViewportCursorMode mode) {
    using SC = we::platform::SystemCursor;
    SC cursor = SC::Arrow;
    switch (mode) {
    case ViewportCursorMode::Orbit: cursor = SC::Crosshair; break;
    case ViewportCursorMode::Pan: cursor = SC::SizeAll; break;
    case ViewportCursorMode::Zoom: cursor = SC::SizeNS; break;
    default: cursor = SC::Arrow; break;
    }
    we::platform::Platform::Get().SetSystemCursor(cursor);
}

void ViewportNavigationController::UpdateCursorMode(bool altDown) {
    ViewportCursorMode next = ViewportCursorMode::Default;
    if (m_FlyLookActive) {
        next = ViewportCursorMode::FlyLook;
    } else if (altDown) {
        if (m_LeftMouseDown) {
            next = ViewportCursorMode::Orbit;
        } else if (m_MiddleMouseDown) {
            next = ViewportCursorMode::Pan;
        } else if (m_RightMouseDown) {
            next = ViewportCursorMode::Zoom;
        }
    }

    if (next != m_CursorMode) {
        m_CursorMode = next;
        ApplySystemCursor(next);
    }
}

void ViewportNavigationController::BeginFlyLook(const Point& cursorPosition) {
    if (!m_Camera || m_Window == we::platform::WindowId::Invalid || m_FlyLookActive) {
        return;
    }

    SaveCursorPosition(cursorPosition);
    m_Camera->EnterFlyMode();
    m_FlyLookActive = true;
    m_IgnoreNextFlyDelta = true;
    m_OrbitDragMode = OrbitDragMode::None;
    auto& platform = we::platform::Platform::Get();
    platform.SetCursorVisible(false);
    CenterFlyCursor();
    platform.SetRelativeMouseMode(m_Window, true);
    m_CursorMode = ViewportCursorMode::FlyLook;
}

void ViewportNavigationController::EndFlyLook() {
    if (!m_Camera || m_Window == we::platform::WindowId::Invalid || !m_FlyLookActive) {
        return;
    }

    auto& platform = we::platform::Platform::Get();
    platform.SetRelativeMouseMode(m_Window, false);
    m_Camera->ExitFlyMode();
    m_FlyLookActive = false;
    RestoreCursorPosition();
    m_IgnoreNextFlyDelta = true;
    m_LastMousePos = m_SavedCursorPos;
    m_CursorMode = ViewportCursorMode::Default;
    platform.SetCursorVisible(true);
    ApplySystemCursor(ViewportCursorMode::Default);
}

void ViewportNavigationController::OnMouseDown(const MouseEvent& event) {
    m_LastMousePos = event.position;

    if (!IsPointerInsideViewport(event.position)) {
        return;
    }

    if (event.button == MouseButton::Left) m_LeftMouseDown = true;
    if (event.button == MouseButton::Right) m_RightMouseDown = true;
    if (event.button == MouseButton::Middle) m_MiddleMouseDown = true;

    if (m_FlyLookActive) {
        return;
    }

    if (event.altDown) {
        if (event.button == MouseButton::Left) {
            m_OrbitDragMode = OrbitDragMode::Orbit;
            if (m_Camera) {
                m_Camera->SetOrbitPivot(ResolveOrbitPivot());
            }
        } else if (event.button == MouseButton::Middle) {
            m_OrbitDragMode = OrbitDragMode::Pan;
        } else if (event.button == MouseButton::Right) {
            m_OrbitDragMode = OrbitDragMode::Dolly;
        }
        UpdateCursorMode(true);
        return;
    }

    if (event.button == MouseButton::Right) {
        BeginFlyLook(event.position);
    }
}

void ViewportNavigationController::OnMouseMove(const MouseEvent& event) {
    if (!m_Camera) {
        return;
    }

    if (m_FlyLookActive) {
        if (m_IgnoreNextFlyDelta) {
            m_IgnoreNextFlyDelta = false;
            m_LastMousePos = event.position;
            return;
        }

        const float dx = event.deltaX;
        const float dy = event.deltaY;
        if (dx != 0.0f || dy != 0.0f) {
            m_Camera->ProcessFlyLook(dx, dy);
        }
        return;
    }

    if (!m_LeftMouseDown && !m_RightMouseDown && !m_MiddleMouseDown) {
        UpdateCursorMode(event.altDown);
        m_LastMousePos = event.position;
        return;
    }

    if (m_IgnoreNextFlyDelta) {
        m_IgnoreNextFlyDelta = false;
        m_LastMousePos = event.position;
        return;
    }

    const float dx = event.position.x - m_LastMousePos.x;
    const float dy = event.position.y - m_LastMousePos.y;

    if (event.altDown) {
        switch (m_OrbitDragMode) {
        case OrbitDragMode::Orbit:
            m_Camera->ProcessMouseOrbit(dx, dy);
            break;
        case OrbitDragMode::Pan:
            m_Camera->ProcessMousePan(dx, dy);
            break;
        case OrbitDragMode::Dolly:
            m_Camera->ProcessMouseDolly(dx * 0.1f);
            break;
        case OrbitDragMode::None:
            break;
        }
    }

    UpdateCursorMode(event.altDown);
    m_LastMousePos = event.position;
}

void ViewportNavigationController::OnMouseUp(const MouseEvent& event) {
    if (event.button == MouseButton::Right) {
        if (m_FlyLookActive) {
            EndFlyLook();
        }
        m_RightMouseDown = false;
        if (m_OrbitDragMode == OrbitDragMode::Dolly) {
            m_OrbitDragMode = OrbitDragMode::None;
        }
    }
    if (event.button == MouseButton::Left) {
        m_LeftMouseDown = false;
        if (m_OrbitDragMode == OrbitDragMode::Orbit) {
            m_OrbitDragMode = OrbitDragMode::None;
        }
    }
    if (event.button == MouseButton::Middle) {
        m_MiddleMouseDown = false;
        if (m_OrbitDragMode == OrbitDragMode::Pan) {
            m_OrbitDragMode = OrbitDragMode::None;
        }
    }

    UpdateCursorMode(event.altDown);
}

void ViewportNavigationController::OnMouseWheel(const MouseEvent& event) {
    if (!m_Camera) {
        return;
    }

    if (m_FlyLookActive) {
        m_Camera->AdjustFlySpeed(event.wheelDeltaY);
        PersistCameraSpeed();
        we::programs::editor::UpdateViewportCameraSpeedIndicator();
        return;
    }

    m_Camera->ProcessMouseScroll(event.wheelDeltaY * 0.5f);
}

void ViewportNavigationController::OnKeyDown(const KeyEvent& event) {
    if (!m_Camera || event.type != KeyEventType::KeyDown) {
        return;
    }

    auto& store = ::we::editor::viewport::ViewportNavigationSettingsStore::Get();
    store.EnsureLoaded();

    if (event.key == we::platform::KeyCode::F && store.GetSettings().focusOnSelection && m_Scene) {
        const int selectedIndex = m_Scene->GetSelectedEntityIndex();
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(m_Scene->GetEntities().size())) {
            const glm::vec3 target = m_Scene->GetEntities()[static_cast<size_t>(selectedIndex)].Position;
            m_Camera->Focus(target);
            m_Camera->SetOrbitPivot(target);
        }
    }
}

void ViewportNavigationController::Tick(float deltaTime) {
    if (!m_Camera || !m_FlyLookActive) {
        return;
    }

    auto& platform = we::platform::Platform::Get();
    using KC = we::platform::KeyCode;
    we::runtime::engine::EditorCameraFlyKeys keys{};
    keys.forward = platform.IsKeyDown(KC::W) || platform.IsKeyDown(KC::Up);
    keys.back = platform.IsKeyDown(KC::S) || platform.IsKeyDown(KC::Down);
    keys.left = platform.IsKeyDown(KC::A) || platform.IsKeyDown(KC::Left);
    keys.right = platform.IsKeyDown(KC::D) || platform.IsKeyDown(KC::Right);
    keys.up = platform.IsKeyDown(KC::E);
    keys.down = platform.IsKeyDown(KC::Q);
    keys.boost = platform.IsKeyDown(KC::LeftShift) || platform.IsKeyDown(KC::RightShift);
    keys.slow = platform.IsKeyDown(KC::LeftControl) || platform.IsKeyDown(KC::RightControl);
    m_Camera->ProcessFlyMovement(keys, deltaTime);
}

void ViewportNavigationController::PersistCameraSpeed() const {
    if (!m_Camera) {
        return;
    }

    auto& store = ::we::editor::viewport::ViewportNavigationSettingsStore::Get();
    store.GetMutableSettings().defaultCameraSpeed = m_Camera->GetCameraSpeed();
    store.Save();
}

} // namespace we::editor::viewport