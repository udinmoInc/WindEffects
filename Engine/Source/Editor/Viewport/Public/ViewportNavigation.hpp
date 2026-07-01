#pragma once

#include "Core/EventSystem.hpp"
#include "EditorCamera.hpp"
#include "Scene/Scene.hpp"
#include <SDL3/SDL.h>
#include <memory>

namespace we::programs::editor {
class ViewportNavigationSettingsStore;
}

namespace we::UI {

enum class ViewportCursorMode {
    Default,
    FlyLook,
    Orbit,
    Pan,
    Zoom
};

class ViewportNavigationController {
public:
    void SetWindow(SDL_Window* window) { m_Window = window; }
    void SetCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera) { m_Camera = camera; }
    void SetScene(const std::shared_ptr<we::runtime::scene::Scene>& scene) { m_Scene = scene; }
    void SetViewportRect(const Rect& rect) { m_ViewportRect = rect; }

    void ApplySettingsFromStore();

    void OnMouseDown(const MouseEvent& event);
    void OnMouseMove(const MouseEvent& event);
    void OnMouseUp(const MouseEvent& event);
    void OnMouseWheel(const MouseEvent& event);
    void OnKeyDown(const KeyEvent& event);
    void Tick(float deltaTime);

    bool IsFlyLookActive() const { return m_FlyLookActive; }
    ViewportCursorMode GetCursorMode() const { return m_CursorMode; }

    void PersistCameraSpeed() const;

private:
    enum class OrbitDragMode {
        None,
        Orbit,
        Pan,
        Dolly
    };

    bool IsPointerInsideViewport(const Point& position) const;
    glm::vec3 ResolveOrbitPivot() const;
    void BeginFlyLook(const Point& cursorPosition);
    void EndFlyLook();
    void UpdateCursorMode(bool altDown);
    void ApplySystemCursor(ViewportCursorMode mode);
    void SaveCursorPosition(const Point& position);
    void RestoreCursorPosition();
    void CenterFlyCursor();

    SDL_Window* m_Window = nullptr;
    std::shared_ptr<we::runtime::engine::EditorCamera> m_Camera;
    std::shared_ptr<we::runtime::scene::Scene> m_Scene;

    Rect m_ViewportRect{};
    Point m_LastMousePos{};
    Point m_SavedCursorPos{};
    bool m_HasSavedCursorPos = false;

    bool m_LeftMouseDown = false;
    bool m_RightMouseDown = false;
    bool m_MiddleMouseDown = false;
    bool m_FlyLookActive = false;
    bool m_IgnoreNextFlyDelta = false;
    OrbitDragMode m_OrbitDragMode = OrbitDragMode::None;
    ViewportCursorMode m_CursorMode = ViewportCursorMode::Default;
};

void ApplyViewportNavigationSettings(
    const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

} // namespace we::UI
