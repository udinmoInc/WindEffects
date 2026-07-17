#pragma once

#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Geometry.h"
#include "KindUI/Input/InputEvents.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"
#include "Platform/Types.h"
#include <memory>

namespace we::editor::viewport {
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::KeyEvent;

enum class ViewportCursorMode {
    Default,
    FlyLook,
    Orbit,
    Pan,
    Zoom
};

class ViewportNavigationController {
public:
    void SetWindow(we::platform::WindowId window) { m_Window = window; }
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
    bool IsViewportNavigating() const {
        return m_FlyLookActive || m_OrbitDragMode != OrbitDragMode::None;
    }
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

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
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

} // namespace we::editor::viewport