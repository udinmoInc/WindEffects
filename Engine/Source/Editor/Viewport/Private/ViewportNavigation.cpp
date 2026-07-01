#include "ViewportNavigation.hpp"
#include "ViewportNavigationSettings.hpp"
#include "ViewportToolbarState.hpp"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_hints.h>
#include <algorithm>

namespace we::UI {

namespace {

we::runtime::engine::EditorCameraNavigationSettings ToCameraSettings(const we::programs::editor::ViewportNavigationSettings& settings) {
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

constexpr const char* kRelativeCursorVisibleHint = "SDL_MOUSE_RELATIVE_CURSOR_VISIBLE";

void SetRelativeCursorVisible(bool visible) {
    SDL_SetHint(kRelativeCursorVisibleHint, visible ? "1" : "0");
}

} // namespace

void ApplyViewportNavigationSettings(
    const std::shared_ptr<we::runtime::engine::EditorCamera>& camera) {
    if (!camera) {
        return;
    }

    auto& store = we::programs::editor::ViewportNavigationSettingsStore::Get();
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
    auto& store = we::programs::editor::ViewportNavigationSettingsStore::Get();
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
    if (!m_Window || !m_HasSavedCursorPos) {
        return;
    }

    SDL_SetWindowMouseRect(m_Window, nullptr);
    SDL_WarpMouseInWindow(m_Window, m_SavedCursorPos.x, m_SavedCursorPos.y);
}

void ViewportNavigationController::CenterFlyCursor() {
    if (!m_Window || m_ViewportRect.width <= 0.0f || m_ViewportRect.height <= 0.0f) {
        return;
    }

    const float centerX = m_ViewportRect.x + m_ViewportRect.width * 0.5f;
    const float centerY = m_ViewportRect.y + m_ViewportRect.height * 0.5f;
    SDL_WarpMouseInWindow(m_Window, centerX, centerY);

    const SDL_Rect mouseRect{
        static_cast<int>(centerX),
        static_cast<int>(centerY),
        1,
        1
    };
    SDL_SetWindowMouseRect(m_Window, &mouseRect);
}

void ViewportNavigationController::ApplySystemCursor(ViewportCursorMode mode) {
    static SDL_Cursor* defaultCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    static SDL_Cursor* crosshairCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    static SDL_Cursor* moveCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    static SDL_Cursor* zoomCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);

    SDL_Cursor* cursor = defaultCursor;
    switch (mode) {
    case ViewportCursorMode::FlyLook:
        cursor = crosshairCursor;
        break;
    case ViewportCursorMode::Orbit:
        cursor = crosshairCursor;
        break;
    case ViewportCursorMode::Pan:
        cursor = moveCursor;
        break;
    case ViewportCursorMode::Zoom:
        cursor = zoomCursor;
        break;
    case ViewportCursorMode::Default:
    default:
        cursor = defaultCursor;
        break;
    }

    if (cursor) {
        SDL_SetCursor(cursor);
    }
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
    if (!m_Camera || !m_Window || m_FlyLookActive) {
        return;
    }

    SaveCursorPosition(cursorPosition);
    m_Camera->EnterFlyMode();
    m_FlyLookActive = true;
    m_IgnoreNextFlyDelta = true;
    m_OrbitDragMode = OrbitDragMode::None;
    ApplySystemCursor(ViewportCursorMode::FlyLook);
    SetRelativeCursorVisible(true);
    SDL_ShowCursor();
    CenterFlyCursor();
    SDL_SetWindowRelativeMouseMode(m_Window, true);
    m_CursorMode = ViewportCursorMode::FlyLook;
}

void ViewportNavigationController::EndFlyLook() {
    if (!m_Camera || !m_Window || !m_FlyLookActive) {
        return;
    }

    SDL_SetWindowRelativeMouseMode(m_Window, false);
    SetRelativeCursorVisible(false);
    m_Camera->ExitFlyMode();
    m_FlyLookActive = false;
    RestoreCursorPosition();
    m_IgnoreNextFlyDelta = true;
    m_LastMousePos = m_SavedCursorPos;
    m_CursorMode = ViewportCursorMode::Default;
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

    auto& store = we::programs::editor::ViewportNavigationSettingsStore::Get();
    store.EnsureLoaded();

    if (event.keycode == SDLK_F && store.GetSettings().focusOnSelection && m_Scene) {
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

    const bool* state = SDL_GetKeyboardState(nullptr);
    bool keys[512] = { false };
    keys[SDL_SCANCODE_W] = state[SDL_SCANCODE_W];
    keys[SDL_SCANCODE_S] = state[SDL_SCANCODE_S];
    keys[SDL_SCANCODE_A] = state[SDL_SCANCODE_A];
    keys[SDL_SCANCODE_D] = state[SDL_SCANCODE_D];
    keys[SDL_SCANCODE_E] = state[SDL_SCANCODE_E];
    keys[SDL_SCANCODE_Q] = state[SDL_SCANCODE_Q];
    keys[SDL_SCANCODE_UP] = state[SDL_SCANCODE_UP];
    keys[SDL_SCANCODE_DOWN] = state[SDL_SCANCODE_DOWN];
    keys[SDL_SCANCODE_LEFT] = state[SDL_SCANCODE_LEFT];
    keys[SDL_SCANCODE_RIGHT] = state[SDL_SCANCODE_RIGHT];
    keys[SDL_SCANCODE_LSHIFT] = state[SDL_SCANCODE_LSHIFT];
    keys[SDL_SCANCODE_RSHIFT] = state[SDL_SCANCODE_RSHIFT];
    keys[SDL_SCANCODE_LCTRL] = state[SDL_SCANCODE_LCTRL];
    keys[SDL_SCANCODE_RCTRL] = state[SDL_SCANCODE_RCTRL];

    m_Camera->ProcessFlyMovement(keys, deltaTime);
}

void ViewportNavigationController::PersistCameraSpeed() const {
    if (!m_Camera) {
        return;
    }

    auto& store = we::programs::editor::ViewportNavigationSettingsStore::Get();
    store.GetMutableSettings().defaultCameraSpeed = m_Camera->GetCameraSpeed();
    store.Save();
}

} // namespace we::UI
